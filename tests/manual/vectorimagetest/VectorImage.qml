// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.VectorImage

Item {
    width: vectorImage.implicitWidth * (VectorImageManager.scale / 10.0)
    height: vectorImage.implicitHeight * (VectorImageManager.scale / 10.0)
    scale: VectorImageManager.scale / 10.0
    transformOrigin: Item.TopLeft


    Image {
        source: "background.png"
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        scale: 1.0 / parent.scale
        width: parent.width
        height: parent.height
        transformOrigin: Item.TopLeft
    }

    VectorImage {
        id: vectorImage
        source: VectorImageManager.currentSource
        preferredRendererType: VectorImage.CurveRenderer
        assumeTrustedSource: true
        animations.loops: VectorImageManager.looping ? Animation.Infinite : 1
    }
}
