// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import Qt.labs.StyleKit

BaseStyle {
    id: style

    /* Properties and Controls left unspecified in a Style will be read from Style.fallbackStyle
     * instead (that is, this file, unless the fallback style is changed). Here we can give the
     * properties some sensible defaults. */

    readonly property real indicatorSize: 24

    control {
        spacing: 5
        padding: 5

        background {
            implicitWidth: 100
            implicitHeight: 40
            border.width: 1
            border.color: "black"
            color: style.palette.base
        }

        indicator {
            implicitWidth: style.indicatorSize
            implicitHeight: style.indicatorSize
            color: style.palette.base
            border.color: "black"
            border.width: 1
            foreground {
                implicitWidth: Style.Stretch
                implicitHeight: Style.Stretch
                margins: 1
                color: style.palette.accent
                image.color: style.palette.accent
                /* Note: don't set implicit size here, since the DelegateContainer will (and should)
                 * fall back to use the size of the image if not set. So if we hard-code a size here,
                 * it cannot be unset to be the size of the image (if any) again from the Style. */
            }
        }

        handle {
            implicitWidth: style.indicatorSize
            implicitHeight: style.indicatorSize
            radius: style.indicatorSize / 2
            border.width: 1
            border.color: "black"
            color: style.palette.window
        }
    }

    /* A control’s text color can be customized for different states and variations.
     * By default, however, it should remain bound to the control’s palette. Otherwise
     * it will fail to respect both the theme palette and any application overrides. */
    control.text.color: palette.buttonText
    textInput.text.color: palette.text

    textInput {
        padding: 5
        background {
            implicitWidth: 150
            implicitHeight: 40
            border.width: 1
            border.color: "black"
            color: style.palette.base
        }
    }

    popup {
        background {
            implicitWidth: 200
            implicitHeight: 200
            border.width: 1
            border.color: "black"
            color: style.palette.base
        }
    }

    pane {
        background {
            implicitWidth: 200
            implicitHeight: 200
            color: style.palette.window
        }
    }

    page {
        background.border.width: 0
    }

    frame {
        background.color: style.palette.base
    }

    groupBox {
        text.padding: 12
        padding: 12
        background.implicitHeight: 20
    }

    flatButton {
        background.visible: false
        hovered.background.visible: true
    }

    checkBox {
        background.visible: false
        indicator {
            foreground {
                visible: false
                color: "transparent"
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
            }
        }
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        checked.indicator.foreground.visible: true
    }

    comboBox {
        text.alignment: Qt.AlignVCenter
        background.implicitWidth: 150
        indicator {
            color: style.palette.base
            border.width: 0
            foreground {
                margins: 4
                color: "transparent"
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/drop-indicator.png"
                image.color: style.palette.buttonText
            }
        }
    }

    spinBox {
        text.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        indicator {
            implicitHeight: Style.Stretch
            color: style.palette.base
            border.width: 0
            margins: 0

            foreground {
                color: "transparent"
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
                image.color: "black"

                // Note: by setting a rotation, we cannot at the same time set the implicit size to
                // Stretch, since Stretch will be based on the unrotated geometry of the indicator.
                // So if the height of the indicator is different from the width, things will look wrong.
                implicitWidth: 14
                implicitHeight: 14
                alignment: Qt.AlignCenter
            }

            down {
                alignment: Qt.AlignLeft
                foreground.rotation: 90
            }

            up {
                alignment: Qt.AlignRight
                foreground.rotation: -90
            }
        }
    }

    radioButton {
        background.visible: false
        indicator {
            radius: 255
            foreground {
                margins: 4
                visible: false
                radius: 255
            }
        }
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        checked.indicator.foreground.visible: true
    }

    itemDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            // Make it flat
            color: palette.base
            border.width: 0
            shadow.visible: false
        }
        hovered.background.color: palette.highlight
    }

    textField {
        text.alignment: Qt.AlignVCenter
        background {
            implicitWidth: 150
            color: style.palette.base
            gradient: null
        }
    }

    progressBar {
        background.visible: false
        indicator.implicitWidth: 150
    }

    scrollBar {
        padding: 4
        background.implicitHeight: 10
        indicator.implicitHeight: 10
        indicator.foreground.color: palette.mid
        pressed.indicator.foreground.color: palette.dark
        vertical {
            background.implicitWidth: 10
            indicator.implicitWidth: 10
        }
    }

    scrollIndicator {
        padding: 1
        background.visible: false
        background.implicitHeight: 10
        indicator.implicitHeight: 10
        indicator.foreground.color: palette.mid
        pressed.indicator.foreground.color: palette.dark
        vertical {
            background.implicitWidth: 10
            indicator.implicitWidth: 10
        }
    }

    slider {
        background {
            visible: false
            implicitWidth: 150
        }
        indicator {
            implicitWidth: Style.Stretch
            implicitHeight: 8
            radius: 8
            foreground {
                radius: 7
            }
        }
    }

    switchControl {
        background.visible: false
        text.alignment: Qt.AlignVCenter
        indicator {
            implicitWidth: style.indicatorSize * 2
            implicitHeight: style.indicatorSize
            alignment: Qt.AlignLeft | Qt.AlignVCenter
            radius: style.indicatorSize / 2
            foreground {
                color: style.palette.base
                radius: style.indicatorSize / 2
            }
        }
        checked.indicator.foreground.color: style.palette.accent
    }

    toolBar {
        background.implicitHeight: 40
    }

    toolSeparator {
        padding: 2
        background.visible: false
        indicator.implicitWidth: 30
        indicator.implicitHeight: 1
        indicator.border.width: 0
        indicator.color: palette.mid
        indicator.foreground.visible: false
    }

    label {
        background.visible: false
        text.color: style.palette.windowText
    }
}
