// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitstorage_p.h"

QT_BEGIN_NAMESPACE

PropertyPathId::PropertyPathId(
    const QQSK::Property property, PropertyPathId_t id, uint numberOfPropertiesInGroup)
    : m_property(property), m_id(id), m_numberOfPropertiesInGroup(numberOfPropertiesInGroup)
{
}

PropertyStorageId PropertyPathId::storageId(QQSK::State state) const
{
    return m_id + (m_numberOfPropertiesInGroup * state);
}

QT_END_NAMESPACE

#include "moc_qqstylekitstorage_p.cpp"
