// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewapplication.h"

#include <QtCore/qfile.h>
#include <QtCore/qthread.h>

#include <cstdlib>

int main(int argc, char *argv[])
{
    // Hack to terminate the event loop when stdin is closed.
    // We want to perform our cleanup and avoid leaking the child process.
    std::unique_ptr<QThread> thread(QThread::create([]() {
        QFile input;
        if (input.open(stdin, QIODevice::ReadOnly))
            input.readAll();
    }));

    int exitCode = -1;
    {
        QmlPreviewApplication app(argc, argv);
        app.parseArguments();
        QObject::connect(thread.get(), &QThread::finished, &app, &app.quit);
        thread->start();
        exitCode = app.exec();
    }

    // _Exit() instead of return so that we don't have to unblock the input reader thread.
    std::_Exit(exitCode);
}
