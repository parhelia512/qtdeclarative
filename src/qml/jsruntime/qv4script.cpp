// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4script_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qv4context_p.h>
#include <private/qv4function_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4profiling_p.h>
#include <private/qv4runtimecodegen_p.h>
#include <private/qv4scopedvalue_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qstring.h>

using namespace QV4;
using namespace QQmlJS;

Script::Script(
        ExecutionEngine *v4, QmlContext *qml,
        const QQmlRefPointer<ExecutableCompilationUnit> &compilationUnit)
    : m_compilationUnit(compilationUnit)
    , m_context(v4->rootContext())
    , m_parseAsBinding(true)
    , m_inheritContext(true)
    , m_parsed(true)
{
    if (qml)
        m_qmlContext.set(v4, *qml);

    m_vmFunction.set(v4, compilationUnit ? compilationUnit->rootFunction() : nullptr);
}

Script::~Script()
{
}

void Script::parse()
{
    if (m_parsed)
        return;

    m_parsed = true;

    ExecutionEngine *v4 = m_context->engine();
    Scope valueScope(v4);

    QV4::Compiler::Module module(m_sourceFile, m_sourceFile, v4->debugger() != nullptr);

    if (m_sourceCode.startsWith(QLatin1String("function("))) {
        static const int snippetLength = 70;
        v4->throwSyntaxError(
                QLatin1String(
                        "Using function expressions as statements in scripts is not compliant "
                        "with the ECMAScript specification:"
                        "\n%1...\n"
                        "If you want a function expression, surround it by parentheses.")
                        .arg(QStringView{m_sourceCode}.left(snippetLength)));
        return;
    }

    Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(m_sourceCode, m_line, m_parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    const auto diagnosticMessages = parser.diagnosticMessages();
    for (const DiagnosticMessage &m : diagnosticMessages) {
        if (m.isError()) {
            valueScope.engine->throwSyntaxError(
                    m.message, m_sourceFile, m.loc.startLine, m.loc.startColumn);
            return;
        } else {
            qWarning() << m_sourceFile << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": warning: " << m.message;
        }
    }

    if (parsed) {
        using namespace AST;
        Program *program = AST::cast<Program *>(parser.rootNode());
        if (!program) {
            // if parsing was successful, and we have no program, then
            // we're done...:
            return;
        }

        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        RuntimeCodegen cg(v4, &jsGenerator, m_strictMode);
        if (m_inheritContext)
            cg.setUseFastLookups(false);
        cg.generateFromProgram(m_sourceCode, program, &module, m_contextType);
        if (v4->hasException)
            return;

        m_compilationUnit = v4->insertCompilationUnit(cg.generateCompilationUnit());
        m_vmFunction.set(v4, m_compilationUnit->rootFunction());
    }

    if (!m_vmFunction) {
        // ### FIX file/line number
        ScopedObject error(valueScope, v4->newSyntaxErrorObject(QStringLiteral("Syntax error")));
        v4->throwError(error);
    }
}

ReturnedValue Script::run(const QV4::Value *thisObject)
{
    if (!m_parsed)
        parse();
    if (!m_vmFunction)
        return Encode::undefined();

    QV4::ExecutionEngine *engine = m_context->engine();
    QV4::Scope valueScope(engine);

    if (m_qmlContext.isUndefined()) {
        QScopedValueRollback<Function*> savedGlobalCode(engine->globalCode, m_vmFunction);

        return m_vmFunction->call(thisObject ? thisObject : engine->globalObject, nullptr, 0,
                                m_context);
    } else {
        Scoped<QmlContext> qml(valueScope, m_qmlContext.value());
        return m_vmFunction->call(thisObject, nullptr, 0, qml);
    }
}

Function *Script::function()
{
    if (!m_parsed)
        parse();
    return m_vmFunction;
}

QQmlRefPointer<QV4::CompiledData::CompilationUnit> Script::precompile(
        QV4::Compiler::Module *module, QQmlJS::Engine *jsEngine,
        Compiler::JSUnitGenerator *unitGenerator, const QString &fileName,
        const QString &source, QList<QQmlError> *reportedErrors,
        QV4::Compiler::ContextType contextType)
{
    using namespace QV4::Compiler;
    using namespace QQmlJS::AST;

    Lexer lexer(jsEngine);
    lexer.setCode(source, /*line*/1, /*qml mode*/false);
    Parser parser(jsEngine);

    parser.parseProgram();

    QList<QQmlError> errors
            = QQmlEnginePrivate::qmlErrorFromDiagnostics(fileName, parser.diagnosticMessages());
    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return nullptr;
    }

    Program *program = AST::cast<Program *>(parser.rootNode());
    if (!program) {
        // if parsing was successful, and we have no program, then
        // we're done...:
        return nullptr;
    }

    Codegen cg(unitGenerator, /*strict mode*/false);
    cg.generateFromProgram(source, program, module, contextType);
    if (cg.hasError()) {
        if (reportedErrors) {
            const auto v4Error = cg.error();
            QQmlError error;
            error.setUrl(cg.url());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(v4Error.loc.startLine));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(v4Error.loc.startColumn));
            error.setDescription(v4Error.message);
            reportedErrors->append(error);
        }
        return nullptr;
    }

    return cg.generateCompilationUnit(/*generate unit data*/false);
}

Script *Script::createFromFileOrCache(
        ExecutionEngine *engine, QmlContext *qmlContext, const QString &fileName,
        const QUrl &originalUrl, QString *error)
{
    if (error)
        error->clear();

    QQmlMetaType::CachedUnitLookupError cacheError = QQmlMetaType::CachedUnitLookupError::NoError;
    const ExecutionEngine::DiskCacheOptions options = engine->diskCacheOptions();
    if (const QQmlPrivate::CachedQmlUnit *cachedUnit
            = (options & ExecutionEngine::DiskCache::Aot)
                ? QQmlMetaType::findCachedCompilationUnit(
                    originalUrl,
                    (options & ExecutionEngine::DiskCache::AotByteCode)
                        ? QQmlMetaType::AcceptUntyped
                        : QQmlMetaType::RequireFullyTyped,
                    &cacheError)
                : nullptr) {
        QQmlRefPointer<QV4::ExecutableCompilationUnit> jsUnit
                = engine->insertCompilationUnit(
                    QQml::makeRefPointer<QV4::CompiledData::CompilationUnit>(
                        cachedUnit->qmlData, cachedUnit->aotCompiledFunctions));
        return new QV4::Script(engine, qmlContext, jsUnit);
    }

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) {
            if (cacheError == QQmlMetaType::CachedUnitLookupError::VersionMismatch) {
                *error = originalUrl.toString()
                        + QString::fromUtf8(" was compiled ahead of time with an incompatible "
                                            "version of Qt and the original source code cannot be "
                                            "found. Please recompile");
            } else {
                *error = QString::fromUtf8("Error opening source file %1: %2")
                                 .arg(originalUrl.toString()).arg(f.errorString());
            }
        }
        return nullptr;
    }

    QByteArray data = f.readAll();
    QString sourceCode = QString::fromUtf8(data);

    auto result = new QV4::Script(
            engine, qmlContext, /*parseAsBinding*/false, sourceCode, originalUrl.toString());
    result->m_contextType = QV4::Compiler::ContextType::ScriptImportedByQML;
    result->parse();
    return result;
}
