// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQSTYLEKITVARIATION_P_H
#define QQSTYLEKITVARIATION_P_H

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

QT_BEGIN_NAMESPACE

class QQStyleKitVariation : public QQStyleKitControls
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Variation)

public:
    QQStyleKitVariation(QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(QQStyleKitVariation)
};

QT_END_NAMESPACE

#endif // QQSTYLEKITVARIATION_P_H
