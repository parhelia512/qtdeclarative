import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        onPressed: (mouse) => window.color = "red"

        TextArea {
            anchors.centerIn: parent
            text: "Hello, World!"
            color: "black"
        }
    }
}
