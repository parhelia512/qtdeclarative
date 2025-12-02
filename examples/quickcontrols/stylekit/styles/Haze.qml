// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    fonts {
        system {
            family: "Courier New"
            pointSize: 12
        }
        textField.bold: true
        label.bold: true
    }

    control {
        // 'control' is the fallback for all the controls. Any properties that are not
        // overridden by a specific control underneath will be read from here instead.
        leftPadding: 10
        topPadding: 5
        rightPadding: 10
        bottomPadding: 5

        handle {
            implicitWidth: 25
            implicitHeight: 25
            radius: 25
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
            shadow {
                opacity: 0.8
                scale: 1.1
            }
        }

        indicator {
            foreground.margins: 2
        }

        transition: Transition {
            StyleKitAnimation {
                animateColors: true
                animateBackgroundShadow: true
                animateHandleShadow: true
                duration: 300
            }
        }

        hovered {
            // For this style, we don't want to show any transitions when entering or while inside
            // the 'hovered' state. This makes the control light up immediately when hovered, but
            // fade out more slowly when returning to the 'normal' state. We therefore override
            // 'transition' and set it to null.
            transition: null
        }
    }

    abstractButton {
        // 'abstractButton' is the fallback for all button types such as 'button', 'checkBox',
        // 'radioButton', 'switch', etc. This is a good place to style the properties they all
        // have in common. Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 100
            implicitHeight: 40
            opacity: 0.8
            radius: 8

            shadow {
                opacity: 0.8
                scale: 1.1
            }

            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
        }
    }

    pane {
        // 'pane' is the fallback for all pane based controls, such as 'frame' and 'groupBox'.
        //  Any properties not set here will fall back to those defined in 'control'.
        background {
            border.width: 0
            implicitWidth: 200
            implicitHeight: 200
            shadow.visible: false
        }
    }

    textInput {
        // 'textInput' is the fallback for all text based controls, such as 'textField', 'textArea',
        // and 'searchField'. Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 200
        }
    }

    button {
        // Here you can override the style for a Button. The properties you set here apply only
        // to a Button, not to for example a CheckBox. Any properties not set here will fall back
        // to those defined in 'abstractButton'.
        pressed {
            background.scale: 0.95
        }
    }

    radioButton {
        indicator {
            foreground {
                margins: 4
                radius: 25 / 2
                border.width: 0
            }
        }
    }

    checkBox {
        transition: Transition {
            NumberAnimation {
                // Using a StyleKitAnimation for transitions is optional. A StyleKitAnimation can be
                // used in parallel with other animations, or not used at all. Here we choose to use
                // a NumberAnimation instead to animate the 'checked' image so that it bounces.
                properties: "indicator.foreground.leftMargin, indicator.foreground.rightMargin"
                            + ", indicator.foreground.topMargin, indicator.foreground.bottomMargin"
                easing.type: Easing.OutBounce
                duration: 500
            }
        }
        hovered {
            transition: null
            indicator.foreground.margins: -15
        }
    }

    spinBox {
        padding: 4
        background {
            implicitWidth: 100
            scale: 1
        }
        // Some indicators have subtypes, such as 'up' and 'down'. Styling them is optional,
        // and any properties left unspecified will fall back to those in 'indicator'.
        indicator.up.color: palette.accent
        indicator.down.color: palette.accent
    }

    comboBox {
        background.implicitWidth: 200
        indicator.foreground.scale: 0.5
        pressed.background.scale: 1.0
    }

    slider {
        background.implicitWidth: 180
        indicator {
            implicitHeight: 8
            radius: 8
            foreground {
                radius: 8
            }
        }
    }

    switchControl {
        indicator {
            implicitWidth: 60
            implicitHeight: 30
            radius: 5
            foreground.radius: 4
        }
        handle {
            leftMargin: 3
            rightMargin: 3
        }
    }

    // You can define one or more Instance Variations that can be enabled from the
    // application using the attached 'StyleKitControl.variations' property.
    // Inside a variation, you list the controls that should receive alternative
    // styling when the variation is active. Any properties defined in a variation
    // override those set in the Style or Theme.
    //
    // For example, if you set "StyleKitControl.variations: ['mini']" on a GroupBox
    // in the application, all controls inside that GroupBox will be affected.
    // Exactly which controls are impacted depends on which ones you style inside
    // the variation.
    Variation {
        name: "mini"

        control {
            padding: 2
            background {
                implicitHeight: 15
            }
            indicator {
                implicitWidth: 15
                implicitHeight: 15
            }
            handle {
                implicitWidth: 15
                implicitHeight: 15
            }
        }

        textInput {
            background.implicitWidth: 100
        }

        abstractButton.background {
            implicitWidth: 60
        }

        switchControl {
            background.implicitWidth: 40
            indicator.implicitWidth: 40
            indicator.implicitHeight: 20
        }

        slider {
            background.implicitWidth: 100
            indicator.implicitHeight: 8
            indicator.implicitWidth: Style.Stretch
        }
    }

    Variation {
        name: "alert"
        abstractButton.background {
            color: "orchid"
            border.color: "orchid"
            shadow.color: "orchid"
        }
    }

    // You can also define Type Variations. Unlike Instance Variations—which apply
    // only to specific control instances—Type Variations are applied to *all*
    // instances of a control type without requiring the application to use attached
    // properties.
    //
    // In this example, we specify that all Buttons that are children of a Frame
    // should receive alternative styling, differentiating them from other Buttons.
    frame {
        background {
            border.width: 1
            shadow.visible: true
        }
        variations: Variation {
            button.background {
                radius: 0
                color: palette.accent
                shadow.visible: false
            }
        }
    }

    readonly property int fancyButton: 0
    CustomControl {
        // You can also define your own custom control types and pair them with custom
        // implementations in your app. Here we provide a base configuration for a control
        // named 'fancyButton', and then override it in the themes to apply colors. Any
        // properties not set here will fall back to those defined in 'control'.
        // The 'controlType' can be any number between 0 and 100000.
        controlType: fancyButton
        background {
            implicitWidth: 200
            radius: 4
        }
    }

    // A style can have any number of themes. The ones assigned to 'light' and 'dark'
    // will be applied according to the current system theme if 'themeName' is set to
    // "System" (the default). Setting the current themeName for a style is usually done
    // from the application rather than from within the style itself.
    //
    // Within a theme, you can override any properties that should have different values
    // when the theme is applied. Typically, a style configures structural properties
    // such as implicit size, padding, and radii, while a theme specifies colors. However,
    // this is not a limitation — any properties can be overridden by a theme. Properties
    // not set in the theme will fall back to those defined in the style.

    light: Theme {
        control {
            background {
                border.color: "white"
                shadow.color: "white"
            }

            handle {
                color: palette.accent
                shadow.color: "white"
                border.color: "white"
            }

            checked {
                background.shadow.color: "white"
                background.color: "blue"
            }

            focused {
                background.border.color: "white"
                background.shadow.color: "white"
            }

            hovered {
                background {
                    color: palette.accent
                    border.color: "white"
                    shadow.color: "white"
                }
                handle {
                    shadow.color: "white"
                    shadow.scale: 1.6
                    border.color: palette.accent.darker(1.4)
                }
            }

            disabled {
                background {
                    opacity: 0.4
                    shadow.visible: false
                    gradient: null
                }
            }
        }

        abstractButton {
            background {
                color: "lightgray"
            }
            hovered.background {
                shadow.scale: 1.4
                color: palette.accent
            }
            checked {
                background.color: palette.accent
            }
        }

        textField {
            background.shadow.scale: 0
            hovered.background.color: "white"
            hovered.background.shadow.scale: 1.2
            focused.background.border.color: palette.accent
        }

        CustomControl {
            controlType: fancyButton
            background {
                color: "lightslategray"
            }
        }

        // In a theme, you should also configure the theme palettes. These palettes serve as
        // the base palettes for the entire application. However, note that the application
        // can override these palettes, and you can even set different palettes per control
        // instance. As a result, there is no single palette for the application; each control
        // can theoretically use its own specific version.
        //
        // For example, when we set "abstractButton.hovered.background.color: palette.accent"
        // above, 'palette' then refers to the palette set on the Qt Quick Button in the application.
        // And this palette can be different from the palettes defined underneath.
        // Note also that we haven't actually specified an accent color in the palettes below,
        // because we want the default color to be picked up from the operating system.

        palettes {
            system.window: "gainsboro"
            textField.text: "#4e4e4e"
            listView.highlight: "blue"
            spinBox.highlight: "lightgray"

            button {
                buttonText: "white"
                highlightedText: "white"
                brightText: "#4e4e4e"
                disabled.buttonText: "#4e4e4e"
                disabled.highlightedText: "#4e4e4e"
            }
        }
    }

    dark: Theme {
        control {
            background {
                border.color: "#3d373b"
                shadow.color: "#404040"
                color: "#8e848a"
            }

            handle {
                border.color: "black"
                shadow.color: "#808080"
            }

            focused {
                background {
                    border.color: "white"
                    shadow.color: "white"
                    color: "#bbbbbb"
                }
            }

            hovered {
                background {
                    border.color: "white"
                    color: palette.accent
                    shadow.color: "white"
                    shadow.scale: 1.1
                    shadow.blur: 20
                }

                handle {
                    shadow.color: "white"
                }
            }

            checked {
                background.color: palette.accent
            }

            disabled {
                background {
                    opacity: 0.3
                    shadow.color: "transparent"
                }
                checked.background.color: "green"
            }
        }

        textInput {
            background.color: "white"
        }

        CustomControl {
            controlType: fancyButton
            background {
                color: "lightsteelblue"
            }
        }

        palettes {
            system.window: "#544e52"
            textField.text: "black"
            spinBox.highlight: "blue"
            button {
                buttonText: "white"
                highlightedText: "white"
                brightText: "white"
                disabled.buttonText: "darkgray"
                disabled.highlightedText: "darkgray"
            }
        }
    }

    // In addition to 'light' and 'dark', you can define as many themes as you want.
    // You can switch between them from the application, for example using:
    // 'StyleKit.style.themeName: "HighContrast"'.

    CustomTheme {
        name: "HighContrast"
        theme: Theme {
            control {
                transition: null

                background {
                    implicitHeight: 40
                    shadow.color: "transparent"
                    color: "ghostwhite"
                    border.color: "black"
                    border.width: 2
                    gradient: null
                }

                indicator {
                    implicitWidth: 30
                    implicitHeight: 30
                    border.color: "black"
                    foreground.margins: 4
                    foreground.color: "black"
                    foreground.image.color: "ghostwhite"
                }

                handle {
                    border.color: "black"
                    border.width: 2
                    implicitWidth: 30
                    implicitHeight: 30
                    radius: 30
                    gradient: null
                }

                hovered {
                    background.border.width: 4
                    indicator.border.width: 4
                    handle.border.width: 4
                }

                checked {
                    background.border.width: 6
                }

                disabled {
                    background.color: "white"
                }
            }

            slider {
                indicator {
                    implicitWidth: 180
                    implicitHeight: 12
                    color: "ghostwhite"
                    border.width: 1
                    foreground.color: "black"
                }
            }

            radioButton {
                indicator.radius: 255
                indicator.foreground.radius: 255
            }

            switchControl {
                background {
                    color: "ghostwhite"
                    border.width: 2
                }

                indicator {
                    radius: 0
                    margins: 0
                    border.width: 2
                    implicitWidth: 60
                    implicitHeight: 40
                    foreground.color: "transparent"
                }

                handle {
                    implicitWidth: 20
                    implicitHeight: 20
                    border.width: 2
                    color: "white"
                    margins: 6
                    radius: 0
                }

                hovered.indicator.border.width: 4
                checked.handle.color: "black"
            }

            spinBox {
                indicator.color: "black"
                indicator.foreground.image.color: "white"
                hovered.background.border.width: 6
            }

            itemDelegate {
                background.border.width: 0
                hovered.background.border.width: 2
                hovered.text.bold: true
            }

            palettes {
                system.window: "white"
                textField.text: "black"
                button {
                    buttonText: "black"
                    highlightedText: "black"
                    brightText: "black"
                    disabled.buttonText: "white"
                    disabled.highlightedText: "white"
                }
            }
        }
    }

    CustomTheme {
        name: "Green"
        theme: Theme {
            control {
                background {
                    border.color: "lightgray"
                    shadow.color: "darkseagreen"
                    color: "#a0c0a0"
                }

                handle {
                    border.color: "lightgray"
                    shadow.color: "darkseagreen"
                    color: "#a0c0a0"
                }
                indicator.color: "darkseagreen"
                indicator.foreground.color: "lightgreen"

                hovered {
                    background {
                        border.color: "lightgreen"
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                    handle {
                        border.color: "lightgreen"
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                }

                checked {
                    background {
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                    indicator {
                        foreground.color: "darkgreen"
                        foreground.image.color: "darkgreen"
                    }
                }

                disabled {
                    background {
                        color: "#80a080"
                        shadow.color: "transparent"
                    }
                }
            }

            switchControl {
                indicator.foreground.color: "darkgreen"
                checked.indicator.foreground.color: "lightgreen"
            }

            checkBox {
                indicator.foreground.color: "transparent"
            }

            comboBox {
                indicator.foreground.color: "transparent"
            }

            Variation {
                name: "alert"
                abstractButton.background {
                    color: "lime"
                    border.color: "lime"
                    shadow.color: "lime"
                }
            }

            palettes {
                system.window: "#547454"
                textField.text: "green"
                checkBox.buttonText: "white"
                button {
                    buttonText: "black"
                    highlightedText: "white"
                    disabled.buttonText: "lightgray"
                    disabled.highlightedText: "lightgray"
                }
            }
        }
    }

    CustomTheme {
        name: "Empty"
        theme: Theme {}
    }
}
