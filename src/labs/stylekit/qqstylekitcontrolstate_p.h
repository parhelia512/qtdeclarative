// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITCONTROLSTATE_P_H
#define QQSTYLEKITCONTROLSTATE_P_H

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

#include "qqstylekitcontrolproperties_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitControl;

class QQStyleKitControlState : public QQStyleKitControlProperties
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitControlState *pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *hovered READ hovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *focused READ focused NOTIFY focusedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *checked READ checked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *disabled READ disabled NOTIFY disabledChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *highlighted READ highlighted NOTIFY highlightedChanged FINAL)
    Q_PROPERTY(QQStyleKitControlState *vertical READ vertical NOTIFY verticalChanged FINAL)
    QML_NAMED_ELEMENT(StyleKitControlState)

public:
    QQStyleKitControlState(QObject *parent = nullptr);

    QQStyleKitControlState *pressed() const;
    QQStyleKitControlState *hovered() const;
    QQStyleKitControlState *focused() const;
    QQStyleKitControlState *checked() const;
    QQStyleKitControlState *disabled() const;
    QQStyleKitControlState *highlighted() const;
    QQStyleKitControlState *vertical() const;

    QQStyleKitControlState *parentState() const;
    std::tuple<QQStyleKitControl *, QQSK::State> controlAndState();

signals:
    void pressedChanged();
    void hoveredChanged();
    void focusedChanged();
    void checkedChanged();
    void disabledChanged();
    void highlightedChanged();
    void verticalChanged();

private:
    Q_DISABLE_COPY(QQStyleKitControlState)

    mutable QPointer<QQStyleKitControlState> m_pressed;
    mutable QPointer<QQStyleKitControlState> m_hovered;
    mutable QPointer<QQStyleKitControlState> m_focused;
    mutable QPointer<QQStyleKitControlState> m_checked;
    mutable QPointer<QQStyleKitControlState> m_disabled;
    mutable QPointer<QQStyleKitControlState> m_highlighted;
    mutable QPointer<QQStyleKitControlState> m_vertical;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITCONTROLSTATE_P_H
