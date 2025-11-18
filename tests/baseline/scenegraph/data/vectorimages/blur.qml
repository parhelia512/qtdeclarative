import QtQuick
import QtQuick.VectorImage

Rectangle{
    id: topLevelItem
    width: 1250
    height: 820

    ListModel {
        id: renderers
        ListElement { renderer: VectorImage.GeometryRenderer }
        ListElement { renderer: VectorImage.CurveRenderer }
    }

    ListModel {
        id: files
        ListElement { src: "../shared/svg/extended_features/blur.svg" }
        ListElement { src: "../shared/svg/extended_features/boxGauss.svg" }
        ListElement { src: "../shared/svg/extended_features/filterandmask.svg" }
    }

    Column {
        spacing: 10
        anchors.fill: parent
        Repeater {
            model: renderers

            Row {
                spacing: 10
                height: 400

                Repeater {
                    model: files

                    VectorImage {
                        source: src
                        preferredRendererType: renderer
                        width: 400
                        height: 400
                        fillMode: VectorImage.PreserveAspectFit
                    }
                }
            }
        }
    }
}
