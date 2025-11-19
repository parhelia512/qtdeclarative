// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITSTORAGE_P_H
#define QQSTYLEKITSTORAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QtCore>

#include "qqstylekitglobal_p.h"

QT_BEGIN_NAMESPACE

using PropertyPathId_t = uint;
using PropertyStorageId = uint;
using QQStyleKitPropertyStorage = QMap<PropertyStorageId, QVariant>;

class PropertyPathId {
    Q_GADGET

public:
    PropertyPathId(
        const QQSK::Property property = QQSK::Property::NoProperty,
        PropertyPathId_t id = 0, uint numberOfPropertiesInGroup = 0);

    QQSK::Property property() const { return m_property; }
    PropertyPathId_t pathId() const { return m_id; }
    PropertyStorageId storageId(QQSK::State state) const;

private:
    QQSK::Property m_property;
    PropertyPathId_t m_id;
    uint m_numberOfPropertiesInGroup;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITSTORAGE_P_H
