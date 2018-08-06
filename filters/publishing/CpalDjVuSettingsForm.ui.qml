import QtQuick 2.0
import QtQuick.Controls 1.2

GroupBox {
    id: root
    title: qsTr("cpaldjvu encoder:")
    width: mainApp.width

    property alias root: root
    property alias cbColors: cbColors
    property alias sbColors: sbColors
    property alias cbBgwhite: cbBgwhite

    Column {
        id: column
        anchors.leftMargin: 6
        anchors.fill: parent

        CheckBox {
            id: cbColors
            text: qsTr("Colors:")
            tooltip: qsTr("Maximum number of colors during quantization (default 256)")
        }

        SpinBox {
            id: sbColors
            enabled: cbColors.checked
            maximumValue: 4096
            minimumValue: 2
            value: 256
            TooltipArea {
                text: cbColors.tooltip
            }
        }

        CheckBox {
            id: cbBgwhite
            text: qsTr("Lightest color is background")
            tooltip: qsTr("Use the lightest color for background (usually white)")
        }
    }
}
