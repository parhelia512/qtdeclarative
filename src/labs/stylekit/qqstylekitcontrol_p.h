// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITCONTROL_P_H
#define QQSTYLEKITCONTROL_P_H

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

#include <QtQml/QtQml>
#include "qqstylekitcontrolstate_p.h"
#include "qqstylekitstorage_p.h"
#include "qqstylekitreader_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitVariation;
class QQStyleKitControlAttached;

class QQStyleKitControl : public QQStyleKitControlState
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQStyleKitVariation> variations READ variations FINAL)
    QML_NAMED_ELEMENT(StyleKitControl)
    QML_ATTACHED(QQStyleKitControlAttached)

public:
    QQStyleKitControl(QObject *parent = nullptr);

    QQmlListProperty<QQStyleKitVariation> variations();

    static QQStyleKitControlAttached *qmlAttachedProperties(QObject *object);

private:
    QVariant readStyleProperty(PropertyStorageId key) const;
    void writeStyleProperty(PropertyStorageId key, const QVariant &value);

private:
    Q_DISABLE_COPY(QQStyleKitControl)

    QList<QQStyleKitVariation *> m_variations;
    mutable QQStyleKitPropertyStorage m_storage;
    QQSK::State m_writtenStates = QQSK::StateFlag::NoState;

    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitControls;
};

class QQStyleKitControlAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList variations READ variations WRITE setVariations NOTIFY variationsChanged FINAL)
    Q_PROPERTY(QQStyleKitExtendedControlType controlType READ controlType WRITE setControlType NOTIFY controlTypeChanged FINAL)

public:
    QQStyleKitControlAttached(QObject *parent);

    QStringList variations() const;
    void setVariations(const QStringList &variations);

    QQStyleKitExtendedControlType controlType();
    void setControlType(QQStyleKitExtendedControlType type);

signals:
    void variationsChanged();
    void controlTypeChanged();

private:
    // m_variations is used for resolving in-app QQStyleKitVariations
    QStringList m_variations;
    // m_controlType is used for resolving in-style QQStyleKitVariations
    QQStyleKitExtendedControlType m_controlType = QQStyleKitReader::ControlType::Unspecified;

    static int s_variationCount;

    friend class QQStyleKit;
    friend class QQStyleKitPropertyResolver;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITCONTROL_P_H
