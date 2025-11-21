import QtQuick
import QtQuick.VectorImage

Rectangle {
    id: topLevelItem
    width: 800
    height: 900

    ListModel {
        id: files
        ListElement { src: "../shared/svg/extended_features/box.svg" }
        ListElement { src: "../shared/svg/extended_features/boxColor.svg" }
        ListElement { src: "../shared/svg/extended_features/fecolormatrix.svg" }
        ListElement { src: "../shared/svg/extended_features/fecolormatrixSimple.svg" }
        ListElement { src: "../shared/svg/extended_features/feoffset.svg" }
    }

    Grid {
        columns: 2
        anchors.fill: parent
        Repeater {
            model: files
            VectorImage {
                source: src
                preferredRendererType: VectorImage.CurveRenderer
                height: 440
                width: 390
                fillMode: VectorImage.PreserveAspectFit
            }
        }
    }
}
