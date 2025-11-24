// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITGLOBAL_P_H
#define QQSTYLEKITGLOBAL_P_H

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

QT_BEGIN_NAMESPACE

using QQStyleKitExtendedControlType = uint;

class QQSK: public QObject
{
    Q_OBJECT

public:
    enum class Delegate {
        NoDelegate              = 0x0000,
        Control                 = 0x0001,
        Background              = 0x0002,
        Handle                  = 0x0004,
        HandleFirst             = 0x0008,
        HandleSecond            = 0x0010,
        Indicator               = 0x0020,
        IndicatorForeground     = 0x0040,
        IndicatorUp             = 0x0080,
        IndicatorUpForeground   = 0x0100,
        IndicatorDown           = 0x0200,
        IndicatorDownForeground = 0x0400,
    };
    Q_DECLARE_FLAGS(Delegates, Delegate)
    Q_FLAG(Delegate)

    enum class PropertyGroup {
        NoGroup,
        Control,
        Background,
        Border,
        DelegateSubType1,
        DelegateSubType2,
        Handle,
        Foreground,
        Image,
        Indicator,
        Shadow,
        globalFlag,
        Text,
        COUNT
    };
    Q_ENUM(PropertyGroup)

    enum class Property {
        NoProperty,
        BottomLeftRadius,
        BottomMargin,
        BottomPadding,
        BottomRightRadius,
        Clip,
        Color,
        Data,
        Delegate,
        FillMode,
        Gradient,
        HOffset,
        Image,
        ImplicitWidth,
        ImplicitHeight,
        LeftMargin,
        LeftPadding,
        Margins,
        MinimumWidth,
        Opacity,
        Padding,
        Radius,
        RightMargin,
        RightPadding,
        Rotation,
        Scale,
        Source,
        Spacing,
        TopLeftRadius,
        TopMargin,
        TopPadding,
        TopRightRadius,
        Transition,
        Variations,
        Visible,
        VOffset,
        Width,
        Blur,
        Alignment,
        COUNT
    };
    Q_ENUM(Property)

    enum class StateFlag {
        NoState     = 0x000,
        Normal      = 0x001,
        Pressed     = 0x002,
        Hovered     = 0x004,
        Highlighted = 0x008,
        Focused     = 0x010,
        Checked     = 0x020,
        Vertical    = 0x040,
        Disabled    = 0x080,
        MAX_STATE   = 0x100,
    };
    Q_DECLARE_FLAGS(State, StateFlag)
    Q_FLAG(State)

    enum class Subclass {
        QQStyleKitState,
        QQStyleKitReader,
    };
    Q_ENUM(Subclass)

    enum class PathFlag {
        NoFlag        = 0x00,
        StyleDirect   = 0x01
    };
    Q_DECLARE_FLAGS(PathFlags, PathFlag)
    Q_FLAG(PathFlag)

public:
    template <typename T, typename Owner, typename... Args>
    static inline T *lazyCreate(T *const &ptr, const Owner *self, Args&&... args)
    {
        if (!ptr) {
            auto *mutableSelf = const_cast<Owner *>(self);
            auto *&mutablePtr = const_cast<T *&>(ptr);
            mutablePtr = new T(std::forward<Args>(args)..., mutableSelf);
        }
        return ptr;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::State)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::Delegates)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::PathFlags)

QT_END_NAMESPACE

#endif // QQSTYLEKITGLOBAL_P_H
