// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitthemeproperties_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitThemeProperties::QQStyleKitThemeProperties(QObject *parent)
    : QQStyleKitControls(parent)
{
}

QQStyleKitFont *QQStyleKitThemeProperties::fonts()
{
    return &m_fonts;
}

QT_END_NAMESPACE

#include "moc_qqstylekitthemeproperties_p.cpp"


