// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITTHEMEPROPERTIES_P_H
#define QQSTYLEKITTHEMEPROPERTIES_P_H

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
#include "qqstylekitcontrols_p.h"
#include "qqstylekitfont_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitThemeProperties : public QQStyleKitControls
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitFont *fonts READ fonts NOTIFY fontsChanged FINAL)
    //TODO: Move palettes property here as well
    QML_UNCREATABLE("This component is abstract, and cannot be instantiated")
    QML_NAMED_ELEMENT(ThemeProperties)

public:
    QQStyleKitThemeProperties(QObject *parent = nullptr);

    QQStyleKitFont *fonts();

signals:
    void fontsChanged();

private:
    Q_DISABLE_COPY(QQStyleKitThemeProperties)

    QQStyleKitFont m_fonts;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITTHEMEPROPERTIES_P_H
