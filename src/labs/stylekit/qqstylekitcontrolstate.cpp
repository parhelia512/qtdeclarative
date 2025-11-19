// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrolstate_p.h"
#include "qqstylekitcontrol_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitControlState::QQStyleKitControlState(QObject *parent)
    : QQStyleKitControlProperties(QQSK::PropertyGroup::Control, parent)
{
}

#define IMPLEMENT_ACCESSORS(STATE) \
QQStyleKitControlState *QQStyleKitControlState::STATE() const \
{ \
    if (!m_ ## STATE) { \
        auto *self = const_cast<QQStyleKitControlState *>(this); \
        self->m_ ## STATE = new QQStyleKitControlState(self); \
    } \
    return m_ ## STATE; \
}

IMPLEMENT_ACCESSORS(pressed);
IMPLEMENT_ACCESSORS(hovered);
IMPLEMENT_ACCESSORS(highlighted);
IMPLEMENT_ACCESSORS(focused);
IMPLEMENT_ACCESSORS(checked);
IMPLEMENT_ACCESSORS(vertical);
IMPLEMENT_ACCESSORS(disabled);

std::tuple<QQStyleKitControl *, QQSK::State>
QQStyleKitControlState::controlAndState()
{
    /* Follow the parent path back to the QQStyleKitControl, and
     * track which nested states we're in along the way. The path of
     * states determines the state of the control that the properties
     * inside this QQStyleKitControlState should apply for. */
    QQStyleKitControl *control = nullptr;
    QQSK::State nestedState = QQSK::StateFlag::NoState;
    const QQStyleKitControlState *obj = this;

    if (metaObject()->inherits(&QQStyleKitControl::staticMetaObject))
        control = asQQStyleKitControl();

    while (true) {
        QQStyleKitControlState *parentState = qobject_cast<QQStyleKitControlState *>(obj->parent());
        if (!parentState)
            break;

        if (obj == parentState->pressed())
            nestedState.setFlag(QQSK::StateFlag::Pressed);
        else if (obj == parentState->hovered())
            nestedState.setFlag(QQSK::StateFlag::Hovered);
        else if (obj == parentState->focused())
            nestedState.setFlag(QQSK::StateFlag::Focused);
        else if (obj == parentState->highlighted())
            nestedState.setFlag(QQSK::StateFlag::Highlighted);
        else if (obj == parentState->checked())
            nestedState.setFlag(QQSK::StateFlag::Checked);
        else if (obj == parentState->vertical())
            nestedState.setFlag(QQSK::StateFlag::Vertical);
        else if (obj == parentState->disabled())
            nestedState.setFlag(QQSK::StateFlag::Disabled);
        else
            Q_UNREACHABLE();

        obj = parentState;
        if (obj->metaObject()->inherits(&QQStyleKitControl::staticMetaObject))
            control = obj->asQQStyleKitControl();
    }

    if (nestedState.testFlag(QQSK::StateFlag::Disabled)) {
        nestedState.setFlag(QQSK::StateFlag::Pressed, false);
        nestedState.setFlag(QQSK::StateFlag::Hovered, false);
        nestedState.setFlag(QQSK::StateFlag::Focused, false);
        nestedState.setFlag(QQSK::StateFlag::Highlighted, false);
    }

    if (nestedState == QQSK::StateFlag::NoState)
        nestedState = QQSK::StateFlag::Normal;

    Q_ASSERT(control);
    Q_ASSERT(qlonglong(nestedState) <= qlonglong(QQSK::StateFlag::MAX_STATE));

    return std::make_tuple(control, nestedState);
}

QQStyleKitControlState *QQStyleKitControlState::parentState() const
{
    Q_ASSERT(subclass() == QQSK::Subclass::QQStyleKitState);
    QObject *p = parent();
    Q_ASSERT(p && qobject_cast<QQStyleKitControlState *>(p));
    return static_cast<QQStyleKitControlState *>(p);
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrolstate_p.cpp"
