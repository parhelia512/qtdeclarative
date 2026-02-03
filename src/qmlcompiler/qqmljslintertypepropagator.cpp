// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslintertypepropagator_p.h"

#include "qqmljsutils_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSLinterTypePropagator::QQmlJSLinterTypePropagator(
        const QV4::Compiler::JSUnitGenerator *unitGenerator,
        const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
        const BasicBlocks &basicBlocks, const InstructionAnnotations &annotations,
        QQmlSA::PassManager *passManager, const ContextPropertyInfo &contextPropertyInfo)
    : QQmlJSTypePropagator(unitGenerator, typeResolver, logger, basicBlocks, annotations, contextPropertyInfo),
      m_passManager(passManager)
{

}

void QQmlJSLinterTypePropagator::generate_Ret()
{
    QQmlJSTypePropagator::generate_Ret();

    if (m_function->isSignalHandler) {
        // Signal handlers cannot return anything.
    } else if (m_state.accumulatorIn().contains(m_typeResolver->voidType())) {
        // You can always return undefined.
    } else if (!m_returnType.isValid() && m_state.accumulatorIn().isValid()) {
        if (m_function->isFullyTyped) {
            // Do not complain if the function didn't have a valid annotation in the first place.
            m_logger->log(u"Function without return type annotation returns %1"_s.arg(
                                  m_state.accumulatorIn().containedTypeName()),
                          qmlIncompatibleType, currentFunctionSourceLocation());
        }
    } else if (!canConvertFromTo(m_state.accumulatorIn(), m_returnType)) {
        m_logger->log(u"Cannot assign binding of type %1 to %2"_s.arg(
                              m_state.accumulatorIn().containedTypeName(),
                              m_returnType.containedTypeName()),
                      qmlIncompatibleType, currentFunctionSourceLocation());
    }

    const QQmlJS::SourceLocation location = m_function->isProperty
            ? currentFunctionSourceLocation()
            : currentNonEmptySourceLocation();
    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeBinding(
                    QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                    QQmlJSScope::createQQmlSAElement(m_state.accumulatorIn().containedType()),
                    QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(location));
}

void QQmlJSLinterTypePropagator::generate_LoadQmlContextPropertyLookup(int index)
{
    QQmlJSTypePropagator::generate_LoadQmlContextPropertyLookup(index);

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);

    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::generate_GetOptionalLookup(int index, int offset)
{
    QQmlJSTypePropagator::generate_GetOptionalLookup(index, offset);

    auto suggMsg = "Consider using non-optional chaining instead: '?.' -> '.'"_L1;
    auto suggestion = std::make_optional(QQmlJSFixSuggestion(suggMsg, currentSourceLocation()));
    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::Enum) {
        m_logger->log("Redundant optional chaining for enum lookup"_L1, qmlRedundantOptionalChaining,
                      currentSourceLocation(), true, true, suggestion);
    } else if (!m_state.accumulatorIn().containedType()->isReferenceType()
               && !m_typeResolver->canHoldUndefined(m_state.accumulatorIn())) {
        auto baseType = m_state.accumulatorIn().containedTypeName();
        m_logger->log("Redundant optional chaining for lookup on non-voidable and non-nullable "_L1
                      "type %1"_L1.arg(baseType), qmlRedundantOptionalChaining,
                      currentSourceLocation(), true, true, suggestion);
    }
}

void QQmlJSLinterTypePropagator::generate_StoreProperty(int nameIndex, int base)
{
    QQmlJSTypePropagator::generate_StoreProperty(nameIndex, base);

    auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);
    const bool isAttached = callBase.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeWrite(
            QQmlJSScope::createQQmlSAElement(callBase.containedType()),
            propertyName,
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            QQmlJSScope::createQQmlSAElement(isAttached
                                                     ? callBase.attachee().containedType()
                                                     : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    QQmlJSTypePropagator::generate_CallProperty(nameIndex, base, argc, argv);

    const auto saCheck = [&](const QString &propertyName, const QQmlJSScope::ConstPtr &baseType) {
        const QQmlSA::Element saBaseType{ QQmlJSScope::createQQmlSAElement(baseType) };
        const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
                m_function->qmlScope.containedType()) };
        const QQmlSA::SourceLocation saLocation{
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
        };

        QQmlSA::PassManagerPrivate::get(m_passManager)
                ->analyzeRead(saBaseType, propertyName, saContainedType, saLocation);
        QQmlSA::PassManagerPrivate::get(m_passManager)
                ->analyzeCall(saBaseType, propertyName, saContainedType, saLocation);
    };

    const auto callBase = m_state.registers[base].content;
    const QString propertyName = m_jsUnitGenerator->stringForIndex(nameIndex);
    const auto member = m_typeResolver->memberType(callBase, propertyName);

    const bool isLoggingMethod = QQmlJSTypePropagator::isLoggingMethod(propertyName);
    if (callBase.contains(m_typeResolver->mathObject()))
        saCheck(propertyName, callBase.containedType());
    else if (callBase.contains(m_typeResolver->consoleObject()) && isLoggingMethod)
        saCheck(propertyName, callBase.containedType());
    else if (!member.isMethod()) {
        if (callBase.contains(m_typeResolver->jsValueType())
            || callBase.contains(m_typeResolver->varType())) {
            saCheck(propertyName, callBase.containedType());
        }
    }
}

void QQmlJSLinterTypePropagator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    QQmlJSTypePropagator::generate_CallPossiblyDirectEval(argc, argv);

    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
    };
    const QQmlSA::Element saBaseType{ QQmlJSScope::createQQmlSAElement(
            m_typeResolver->jsGlobalObject()) };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, "eval"_L1, saContainedType, saLocation);
}

void QQmlJSLinterTypePropagator::handleLookupError(const QString &propertyName)
{
    QQmlJSTypePropagator::handleLookupError(propertyName);

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
    const QString typeName = accumulatorIn.containedTypeName();

    if (typeName == u"QVariant")
        return;
    if (accumulatorIn.isList() && propertyName == u"length")
        return;

    auto baseType = accumulatorIn.containedType();
    // Warn separately when a property is only not found because of a missing type

    if (propertyResolution(baseType, propertyName) != PropertyMissing)
        return;

    if (baseType->isScript())
        return;

    std::optional<QQmlJSFixSuggestion> fixSuggestion;

    if (auto suggestion = QQmlJSUtils::didYouMean(propertyName, baseType->properties().keys(),
                                                  currentSourceLocation());
        suggestion.has_value()) {
        fixSuggestion = suggestion;
    }

    if (!fixSuggestion.has_value()
        && accumulatorIn.variant() == QQmlJSRegisterContent::MetaType) {

        const QQmlJSScope::ConstPtr scopeType = accumulatorIn.scopeType();
        const auto metaEnums = scopeType->enumerations();
        const bool enforcesScoped = scopeType->enforcesScopedEnums();

        QStringList enumKeys;
        for (const QQmlJSMetaEnum &metaEnum : metaEnums) {
            if (!enforcesScoped || !metaEnum.isScoped())
                enumKeys << metaEnum.keys();
        }

        if (auto suggestion = QQmlJSUtils::didYouMean(
                    propertyName, enumKeys, currentSourceLocation());
            suggestion.has_value()) {
            fixSuggestion = suggestion;
        }
    }

    m_logger->log(u"Member \"%1\" not found on type \"%2\""_s.arg(propertyName, typeName),
                  qmlMissingProperty, currentSourceLocation(), true, true, fixSuggestion);
}

void QQmlJSLinterTypePropagator::generate_StoreNameCommon(int nameIndex)
{
    QQmlJSTypePropagator::generate_StoreNameCommon(nameIndex);

    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const bool isAttached = in.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            name,
            QQmlJSScope::createQQmlSAElement(isAttached
                    ? in.attachee().containedType()
                    : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));

    const auto qmlScope = m_function->qmlScope.containedType();
    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeWrite(
            QQmlJSScope::createQQmlSAElement(qmlScope), name,
            QQmlJSScope::createQQmlSAElement(in.containedType()),
            QQmlJSScope::createQQmlSAElement(qmlScope),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::propagatePropertyLookup(const QString &name, int lookupIndex)
{
    QQmlJSTypePropagator::propagatePropertyLookup(name, lookupIndex);

    const QQmlJSRegisterContent in = m_state.accumulatorIn();
    const bool isAttached = in.variant() == QQmlJSRegisterContent::Attachment;

    QQmlSA::PassManagerPrivate::get(m_passManager)->analyzeRead(
            QQmlJSScope::createQQmlSAElement(
                    m_state.accumulatorIn().containedType()),
            name,
            QQmlJSScope::createQQmlSAElement(isAttached
                    ? in.attachee().containedType()
                    : m_function->qmlScope.containedType()),
            QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                    currentNonEmptySourceLocation()));
}

void QQmlJSLinterTypePropagator::propagateCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
                                               QQmlJSRegisterContent scope)
{
    QQmlJSTypePropagator::propagateCall(methods, argc, argv, scope);

    QStringList errors;
    const QQmlJSMetaMethod match = bestMatchForCall(methods, argc, argv, &errors);
    if (!match.isValid())
        return;

    const QQmlSA::Element saBaseType = QQmlJSScope::createQQmlSAElement(scope.containedType());
    const QQmlSA::SourceLocation saLocation{
        QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(currentSourceLocation())
    };
    const QQmlSA::Element saContainedType{ QQmlJSScope::createQQmlSAElement(
            m_function->qmlScope.containedType()) };

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(saBaseType, match.methodName(), saContainedType, saLocation);
}

void QQmlJSLinterTypePropagator::propagateTranslationMethod_SAcheck(const QString &methodName)
{
    QQmlJSTypePropagator::propagateTranslationMethod_SAcheck(methodName);

    QQmlSA::PassManagerPrivate::get(m_passManager)
            ->analyzeCall(QQmlJSScope::createQQmlSAElement(m_typeResolver->jsGlobalObject()),
                          methodName,
                          QQmlJSScope::createQQmlSAElement(m_function->qmlScope.containedType()),
                          QQmlSA::SourceLocationPrivate::createQQmlSASourceLocation(
                                  currentNonEmptySourceLocation()));
}

QT_END_NAMESPACE
