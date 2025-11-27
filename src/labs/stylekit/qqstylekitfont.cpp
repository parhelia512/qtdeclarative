// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitfont_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitFont::QQStyleKitFont(QObject *parent)
    : QObject(parent)
{
}

#define DEFINE_FONT_GETTER(scopeName, scopeEnum) \
    QFont QQStyleKitFont::scopeName##Font() const \
    { \
        return fontForScope(QQuickTheme::scopeEnum); \
    }

DEFINE_FONT_GETTER(system, System)
DEFINE_FONT_GETTER(button, Button)
DEFINE_FONT_GETTER(checkBox, CheckBox)
DEFINE_FONT_GETTER(comboBox, ComboBox)
DEFINE_FONT_GETTER(groupBox, GroupBox)
DEFINE_FONT_GETTER(itemView, ItemView)
DEFINE_FONT_GETTER(label, Label)
DEFINE_FONT_GETTER(listView, ListView)
DEFINE_FONT_GETTER(menu, Menu)
DEFINE_FONT_GETTER(menuBar, MenuBar)
DEFINE_FONT_GETTER(radioButton, RadioButton)
DEFINE_FONT_GETTER(spinBox, SpinBox)
DEFINE_FONT_GETTER(switchControl, Switch)
DEFINE_FONT_GETTER(tabBar, TabBar)
DEFINE_FONT_GETTER(textArea, TextArea)
DEFINE_FONT_GETTER(textField, TextField)
DEFINE_FONT_GETTER(toolBar, ToolBar)
DEFINE_FONT_GETTER(toolTip, ToolTip)
DEFINE_FONT_GETTER(tumbler, Tumbler)

#define DEFINE_FONT_SETTER(scopeName, scopeEnum, signal) \
    void QQStyleKitFont::set##scopeName##Font(const QFont &font) \
    { \
        setFontForScope(QQuickTheme::scopeEnum, font, &QQStyleKitFont::signal); \
    }

DEFINE_FONT_SETTER(System, System, systemFontChanged)
DEFINE_FONT_SETTER(Button, Button, buttonFontChanged)
DEFINE_FONT_SETTER(CheckBox, CheckBox, checkBoxFontChanged)
DEFINE_FONT_SETTER(ComboBox, ComboBox, comboBoxFontChanged)
DEFINE_FONT_SETTER(GroupBox, GroupBox, groupBoxFontChanged)
DEFINE_FONT_SETTER(ItemView, ItemView, itemViewFontChanged)
DEFINE_FONT_SETTER(Label, Label, labelFontChanged)
DEFINE_FONT_SETTER(ListView, ListView, listViewFontChanged)
DEFINE_FONT_SETTER(Menu, Menu, menuFontChanged)
DEFINE_FONT_SETTER(MenuBar, MenuBar, menuBarFontChanged)
DEFINE_FONT_SETTER(RadioButton, RadioButton, radioButtonFontChanged)
DEFINE_FONT_SETTER(SpinBox, SpinBox, spinBoxFontChanged)
DEFINE_FONT_SETTER(SwitchControl, Switch, switchControlFontChanged)
DEFINE_FONT_SETTER(TabBar, TabBar, tabBarFontChanged)
DEFINE_FONT_SETTER(TextArea, TextArea, textAreaFontChanged)
DEFINE_FONT_SETTER(TextField, TextField, textFieldFontChanged)
DEFINE_FONT_SETTER(ToolBar, ToolBar, toolBarFontChanged)
DEFINE_FONT_SETTER(ToolTip, ToolTip, toolTipFontChanged)
DEFINE_FONT_SETTER(Tumbler, Tumbler, tumblerFontChanged)

void QQStyleKitFont::setFontForScope(QQuickTheme::Scope scope, const QFont &font, void (QQStyleKitFont::*signal)())
{
    const int index = int(scope);
    if (isSet(scope) && m_local[index] == font)
        return;

    QFont local = font;
    // TODO: Figure out resolve mask to set here

    m_local[index] = font;
    markSet(scope);

    m_effectiveDirty = true;

    emit (this->*signal)();
}

// The fallback font is used to resolve unset fonts
// The theme fonts fallback to the style fonts and
// style fonts fallback to the fallback style fonts
QQStyleKitFont *QQStyleKitFont::fallbackFont() const
{
    return m_fallback;
}

void QQStyleKitFont::setFallbackFont(QQStyleKitFont *fallback)
{
    if (m_fallback == fallback)
        return;

    if (m_fallback)
        disconnect(m_fallback, nullptr, this, nullptr);

    m_fallback = fallback;

    markEffectiveDirty();
    ensureEffectiveUpToDate();

    if (m_fallback) {
        auto makeHandler = [this](void (QQStyleKitFont::*signal)()) {
            return [this, signal] {
                markEffectiveDirty();
                emit (this->*signal)();
            };
        };
        connect(m_fallback, &QQStyleKitFont::systemFontChanged, this,
            makeHandler(&QQStyleKitFont::systemFontChanged));
        connect(m_fallback, &QQStyleKitFont::buttonFontChanged, this,
                makeHandler(&QQStyleKitFont::buttonFontChanged));
        connect(m_fallback, &QQStyleKitFont::checkBoxFontChanged, this,
                makeHandler(&QQStyleKitFont::checkBoxFontChanged));
        connect(m_fallback, &QQStyleKitFont::comboBoxFontChanged, this,
                makeHandler(&QQStyleKitFont::comboBoxFontChanged));
        connect(m_fallback, &QQStyleKitFont::groupBoxFontChanged, this,
                makeHandler(&QQStyleKitFont::groupBoxFontChanged));
        connect(m_fallback, &QQStyleKitFont::itemViewFontChanged, this,
                makeHandler(&QQStyleKitFont::itemViewFontChanged));
        connect(m_fallback, &QQStyleKitFont::labelFontChanged, this,
                makeHandler(&QQStyleKitFont::labelFontChanged));
        connect(m_fallback, &QQStyleKitFont::listViewFontChanged, this,
                makeHandler(&QQStyleKitFont::listViewFontChanged));
        connect(m_fallback, &QQStyleKitFont::menuFontChanged, this,
                makeHandler(&QQStyleKitFont::menuFontChanged));
        connect(m_fallback, &QQStyleKitFont::menuBarFontChanged, this,
                makeHandler(&QQStyleKitFont::menuBarFontChanged));
        connect(m_fallback, &QQStyleKitFont::radioButtonFontChanged, this,
                makeHandler(&QQStyleKitFont::radioButtonFontChanged));
        connect(m_fallback, &QQStyleKitFont::spinBoxFontChanged, this,
                makeHandler(&QQStyleKitFont::spinBoxFontChanged));
        connect(m_fallback, &QQStyleKitFont::switchControlFontChanged, this,
                makeHandler(&QQStyleKitFont::switchControlFontChanged));
        connect(m_fallback, &QQStyleKitFont::tabBarFontChanged, this,
                makeHandler(&QQStyleKitFont::tabBarFontChanged));
        connect(m_fallback, &QQStyleKitFont::textAreaFontChanged, this,
                makeHandler(&QQStyleKitFont::textAreaFontChanged));
        connect(m_fallback, &QQStyleKitFont::textFieldFontChanged, this,
                makeHandler(&QQStyleKitFont::textFieldFontChanged));
        connect(m_fallback, &QQStyleKitFont::toolBarFontChanged, this,
                makeHandler(&QQStyleKitFont::toolBarFontChanged));
        connect(m_fallback, &QQStyleKitFont::toolTipFontChanged, this,
                makeHandler(&QQStyleKitFont::toolTipFontChanged));
        connect(m_fallback, &QQStyleKitFont::tumblerFontChanged, this,
                makeHandler(&QQStyleKitFont::tumblerFontChanged));
    }
    emit fallbackFontChanged();
}

QFont QQStyleKitFont::fontForScope(QQuickTheme::Scope scope) const
{
    ensureEffectiveUpToDate();
    return m_effective[int(scope)];
}

void QQStyleKitFont::ensureEffectiveUpToDate() const
{
    if (!m_effectiveDirty)
        return;

    const int sysIdx = int(QQuickTheme::System);

    {
        const QFont localSys = isSet(QQuickTheme::System) ? m_local[sysIdx] : QFont();
        const QFont fbSys = m_fallback ? m_fallback->fontForScope(QQuickTheme::System) : QFont();
        // TODO: Resolve mask?
        m_effective[sysIdx] = localSys.resolve(fbSys);

    }

    const QFont systemEff = m_effective[sysIdx];
    const QFont fallbackSystem = m_fallback ? m_fallback->fontForScope(QQuickTheme::System) : QFont();

    // Scopes: localScope > localSystem > fallbackScope > fallbackSystem
    for (int i = 0; i < NScopes; ++i) {
        if (i == sysIdx)
            continue;

        const QQuickTheme::Scope scope = static_cast<QQuickTheme::Scope>(i);
        const QFont localRole = isSet(scope) ? m_local[i] : QFont();
        const QFont fallbackRole = m_fallback ? m_fallback->fontForScope(scope) : QFont();

        QFont fallbackLayer = fallbackRole.resolve(fallbackSystem);
        QFont base = systemEff.resolve(fallbackLayer);
        // TODO: Resolve mask?
        m_effective[i] = localRole.resolve(base);
    }

    m_effectiveDirty = false;
}

QT_END_NAMESPACE
#include "moc_qqstylekitfont_p.cpp"
