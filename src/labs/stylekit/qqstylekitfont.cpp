// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitfont_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitFont::QQStyleKitFont(QObject *parent)
    : QObject(parent)
{
}

#define STYLEKIT_FONT_GETTER(propertyName) \
    QFont QQStyleKitFont::propertyName() const \
    {                                         \
        if (!m_##propertyName) {              \
            m_##propertyName.reset(new QFont()); \
        }                                     \
        return *m_##propertyName;             \
    }

STYLEKIT_FONT_GETTER(systemFont)
STYLEKIT_FONT_GETTER(buttonFont)
STYLEKIT_FONT_GETTER(checkboxFont)
STYLEKIT_FONT_GETTER(comboBoxFont)
STYLEKIT_FONT_GETTER(groupBoxFont)
STYLEKIT_FONT_GETTER(itemViewFont)
STYLEKIT_FONT_GETTER(labelFont)
STYLEKIT_FONT_GETTER(listViewFont)
STYLEKIT_FONT_GETTER(menuFont)
STYLEKIT_FONT_GETTER(menuBarFont)
STYLEKIT_FONT_GETTER(radioButtonFont)
STYLEKIT_FONT_GETTER(spinBoxFont)
STYLEKIT_FONT_GETTER(switchControlFont)
STYLEKIT_FONT_GETTER(tabBarFont)
STYLEKIT_FONT_GETTER(textAreaFont)
STYLEKIT_FONT_GETTER(textFieldFont)
STYLEKIT_FONT_GETTER(toolBarFont)
STYLEKIT_FONT_GETTER(toolTipFont)
STYLEKIT_FONT_GETTER(tumblerFont)

void QQStyleKitFont::setSystemFont(const QFont &font)
{
    if (!m_systemFont)
        m_systemFont.reset(new QFont(font));
    else
        *m_systemFont = font;
    emit systemFontChanged();
}

void QQStyleKitFont::setButtonFont(const QFont &font)
{
    if (!m_buttonFont) {
        m_buttonFont.reset(new QFont(font));
    } else {
        *m_buttonFont = font;
    }
    emit buttonFontChanged();
}

void QQStyleKitFont::setCheckboxFont(const QFont &font)
{
    if (!m_checkboxFont) {
        m_checkboxFont.reset(new QFont(font));
    } else {
        *m_checkboxFont = font;
    }
    emit checkboxFontChanged();
}

void QQStyleKitFont::setComboBoxFont(const QFont &font)
{
    if (!m_comboBoxFont) {
        m_comboBoxFont.reset(new QFont(font));
    } else {
        *m_comboBoxFont = font;
    }
    emit comboBoxFontChanged();
}

void QQStyleKitFont::setGroupBoxFont(const QFont &font)
{
    if (!m_groupBoxFont) {
        m_groupBoxFont.reset(new QFont(font));
    } else {
        *m_groupBoxFont = font;
    }
    emit groupBoxFontChanged();
}

void QQStyleKitFont::setItemViewFont(const QFont &font)
{
    if (!m_itemViewFont) {
        m_itemViewFont.reset(new QFont(font));
    } else {
        *m_itemViewFont = font;
    }
    emit itemViewFontChanged();
}

void QQStyleKitFont::setLabelFont(const QFont &font)
{
    if (!m_labelFont) {
        m_labelFont.reset(new QFont(font));
    } else {
        *m_labelFont = font;
    }
    emit labelFontChanged();
}

void QQStyleKitFont::setListViewFont(const QFont &font)
{
    if (!m_listViewFont) {
        m_listViewFont.reset(new QFont(font));
    } else {
        *m_listViewFont = font;
    }
    emit listViewFontChanged();
}

void QQStyleKitFont::setMenuFont(const QFont &font)
{
    if (!m_menuFont) {
        m_menuFont.reset(new QFont(font));
    } else {
        *m_menuFont = font;
    }
    emit menuFontChanged();
}

void QQStyleKitFont::setMenuBarFont(const QFont &font)
{
    if (!m_menuBarFont) {
        m_menuBarFont.reset(new QFont(font));
    } else {
        *m_menuBarFont = font;
    }
    emit menuBarFontChanged();
}

void QQStyleKitFont::setRadioButtonFont(const QFont &font)
{
    if (!m_radioButtonFont) {
        m_radioButtonFont.reset(new QFont(font));
    } else {
        *m_radioButtonFont = font;
    }
    emit radioButtonFontChanged();
}

void QQStyleKitFont::setSpinBoxFont(const QFont &font)
{
    if (!m_spinBoxFont) {
        m_spinBoxFont.reset(new QFont(font));
    } else {
        *m_spinBoxFont = font;
    }
    emit spinBoxFontChanged();
}

void QQStyleKitFont::setSwitchControlFont(const QFont &font)
{
    if (!m_switchControlFont) {
        m_switchControlFont.reset(new QFont(font));
    } else {
        *m_switchControlFont = font;
    }
    emit switchControlFontChanged();
}

void QQStyleKitFont::setTabBarFont(const QFont &font)
{
    if (!m_tabBarFont) {
        m_tabBarFont.reset(new QFont(font));
    } else {
        *m_tabBarFont = font;
    }
    emit tabBarFontChanged();
}

void QQStyleKitFont::setTextAreaFont(const QFont &font)
{
    if (!m_textAreaFont) {
        m_textAreaFont.reset(new QFont(font));
    } else {
        *m_textAreaFont = font;
    }
    emit textAreaFontChanged();
}

void QQStyleKitFont::setTextFieldFont(const QFont &font)
{
    if (!m_textFieldFont) {
        m_textFieldFont.reset(new QFont(font));
    } else {
        *m_textFieldFont = font;
    }
    emit textFieldFontChanged();
}

void QQStyleKitFont::setToolBarFont(const QFont &font)
{
    if (!m_toolBarFont) {
        m_toolBarFont.reset(new QFont(font));
    } else {
        *m_toolBarFont = font;
    }
    emit toolBarFontChanged();
}

void QQStyleKitFont::setToolTipFont(const QFont &font)
{
    if (!m_toolTipFont) {
        m_toolTipFont.reset(new QFont(font));
    } else {
        *m_toolTipFont = font;
    }
    emit toolTipFontChanged();
}

void QQStyleKitFont::setTumblerFont(const QFont &font)
{
    if (!m_tumblerFont) {
        m_tumblerFont.reset(new QFont(font));
    } else {
        *m_tumblerFont = font;
    }
    emit tumblerFontChanged();
}

QT_END_NAMESPACE
#include "moc_qqstylekitfont_p.cpp"
