# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_AUTOMOC true)

qt_add_qml_module(tst_qqmlimport_static_module
    URI MyStaticModule
    STATIC
    QML_FILES
        SomeComponent.qml
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
)

qt_internal_extend_target(tst_qqmlimport_static_moduleplugin
    SOURCES
        mystaticmoduleplugin.h
)
