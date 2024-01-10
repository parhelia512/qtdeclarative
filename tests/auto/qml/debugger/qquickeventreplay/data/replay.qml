// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Window {
    visible: true
    width: 100
    height: 100
    MouseArea {
        focus: true
        anchors.fill: parent
        onClicked: (mouse) => console.log("mouse", mouse.x, mouse.y)
        Keys.onPressed: (key) => console.log("key", key.key, "pressed")
        Keys.onReleased: (key) => console.log("key", key.key, "released")
    }
}
