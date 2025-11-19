// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpalette_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitPalette::QQStyleKitPalette(QObject *parent)
    : QObject(parent)
{
}

#define UNIFIED_PALETTE_GETTER(PROP) \
QQuickPalette *QQStyleKitPalette::PROP() const { \
    if (!m_ ## PROP) \
        m_ ## PROP.reset(new QQuickPalette()); \
    return m_ ## PROP.get(); \
}

UNIFIED_PALETTE_GETTER(system)
UNIFIED_PALETTE_GETTER(checkBox)
UNIFIED_PALETTE_GETTER(button)
UNIFIED_PALETTE_GETTER(comboBox)
UNIFIED_PALETTE_GETTER(groupBox)
UNIFIED_PALETTE_GETTER(itemView)
UNIFIED_PALETTE_GETTER(label)
UNIFIED_PALETTE_GETTER(listView)
UNIFIED_PALETTE_GETTER(menu)
UNIFIED_PALETTE_GETTER(menuBar)
UNIFIED_PALETTE_GETTER(radioButton)
UNIFIED_PALETTE_GETTER(spinBox)
UNIFIED_PALETTE_GETTER(switchControl)
UNIFIED_PALETTE_GETTER(tabBar)
UNIFIED_PALETTE_GETTER(textArea)
UNIFIED_PALETTE_GETTER(textField)
UNIFIED_PALETTE_GETTER(toolBar)
UNIFIED_PALETTE_GETTER(toolTip)
UNIFIED_PALETTE_GETTER(tumbler)

QT_END_NAMESPACE

#include "moc_qqstylekitpalette_p.cpp"
