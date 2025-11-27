// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include "qqstylekittheme_p.h"
#include "qqstylekitstyle_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitTheme::QQStyleKitTheme(QObject *parent)
    : QQStyleKitStyleAndThemeBase(parent)
{
}

QQStyleKitStyle *QQStyleKitTheme::style() const
{
    QObject *parentObj = parent();
    if (!parentObj)
        return nullptr;
    Q_ASSERT(qobject_cast<QQStyleKitStyle *>(parentObj));
    return static_cast<QQStyleKitStyle *>(parentObj);
}

QQStyleKitPalette *QQStyleKitTheme::palettes()
{
    return &m_palettes;
}

// Copied from QQuickTheme
static QPlatformTheme::Palette toPlatformThemePalette(QQuickTheme::Scope scope)
{
    switch (scope) {
    case QQuickTheme::Button: return QPlatformTheme::ButtonPalette;
    case QQuickTheme::CheckBox: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::ComboBox: return QPlatformTheme::ComboBoxPalette;
    case QQuickTheme::GroupBox: return QPlatformTheme::GroupBoxPalette;
    case QQuickTheme::ItemView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Label: return QPlatformTheme::LabelPalette;
    case QQuickTheme::ListView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Menu: return QPlatformTheme::MenuPalette;
    case QQuickTheme::MenuBar: return QPlatformTheme::MenuBarPalette;
    case QQuickTheme::RadioButton: return QPlatformTheme::RadioButtonPalette;
    case QQuickTheme::SpinBox: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::Switch: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::TabBar: return QPlatformTheme::TabBarPalette;
    case QQuickTheme::TextArea: return QPlatformTheme::TextEditPalette;
    case QQuickTheme::TextField: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::ToolBar: return QPlatformTheme::ToolButtonPalette;
    case QQuickTheme::ToolTip: return QPlatformTheme::ToolTipPalette;
    case QQuickTheme::Tumbler: return QPlatformTheme::ItemViewPalette;
    default: return QPlatformTheme::SystemPalette;
    }
}

void QQStyleKitTheme::updateThemePalette()
{
    auto *theme = QQuickTheme::instance();
    if (!theme)
        return;

    // QQuickTheme currently offers only one default platform palette for all controls.
    // This implementation will instead inherit the corresponding platform palette for each
    // control type. Hence, for now, we don't use QQuickTheme::usePlatformPalette, but roll
    // our own version instead.
    theme->setUsePlatformPalette(false);

    const auto *platformTheme = QGuiApplicationPrivate::platformTheme();

#define SET_PALETTE(CONTROL, SCOPE) { \
    const QQuickPalette *controlPalette = m_palettes.CONTROL(); \
        const QPalette *platformPalette = platformTheme->palette(toPlatformThemePalette(SCOPE)); \
        if (controlPalette && platformPalette) { \
            QPalette resolved = controlPalette->toQPalette().resolve(*platformPalette); \
            theme->setPalette(SCOPE, resolved); \
    } else if (platformPalette) { \
            theme->setPalette(SCOPE, *platformPalette); \
    } else if (controlPalette) { \
            theme->setPalette(SCOPE, controlPalette->toQPalette()); \
    } \
}

    SET_PALETTE(system, QQuickTheme::System);
    SET_PALETTE(button, QQuickTheme::Button);
    SET_PALETTE(checkBox, QQuickTheme::CheckBox);
    SET_PALETTE(comboBox, QQuickTheme::ComboBox);
    SET_PALETTE(groupBox, QQuickTheme::GroupBox);
    SET_PALETTE(itemView, QQuickTheme::ItemView);
    SET_PALETTE(label, QQuickTheme::Label);
    SET_PALETTE(listView, QQuickTheme::ListView);
    SET_PALETTE(menu, QQuickTheme::Menu);
    SET_PALETTE(menuBar, QQuickTheme::MenuBar);
    SET_PALETTE(radioButton, QQuickTheme::RadioButton);
    SET_PALETTE(spinBox, QQuickTheme::SpinBox);
    SET_PALETTE(switchControl, QQuickTheme::Switch);
    SET_PALETTE(tabBar, QQuickTheme::TabBar);
    SET_PALETTE(textArea, QQuickTheme::TextArea);
    SET_PALETTE(textField, QQuickTheme::TextField);
    SET_PALETTE(toolBar, QQuickTheme::ToolBar);
    SET_PALETTE(toolTip, QQuickTheme::ToolTip);
    SET_PALETTE(tumbler, QQuickTheme::Tumbler);

    QEvent event(QEvent::ApplicationPaletteChange);
    QGuiApplication::sendEvent(qGuiApp, &event);
}

void QQStyleKitTheme::updateQuickTheme()
{
    updateThemePalette();
}

void QQStyleKitTheme::componentComplete()
{
    QQStyleKitControls::componentComplete();
    m_completed = true;
}

QT_END_NAMESPACE

#include "moc_qqstylekittheme_p.cpp"
