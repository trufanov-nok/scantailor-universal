import QtQuick 2.0
import QtQuick.Controls 1.2

GroupBox {
    id: root

    title: qsTr("cjb4 encoder:")
    width: mainApp.width

    property alias root: root
    property alias cbClean: cbClean
    property alias cbLossy: cbLossy
    property alias sbLossy: sbLossy
    property alias tooltipArea: tooltipArea
    property int __lossy_level_default__: 70

    Column {
        id: column
        anchors.fill: parent

        CheckBox {
            id: cbClean
            anchors.left: parent.left
            anchors.leftMargin: 6
            text: qsTr("Clean")
            checked: true
            tooltip: qsTr("Cleanup image by removing small flyspecks")
        }

        CheckBox {
            id: cbLossy
            enabled: cbClean.checked
            anchors.left: parent.left
            anchors.leftMargin: 6
            text: qsTr("Lossy")
            checked: true
            tooltip: qsTr("Lossy compression (implies -clean as well)")
        }

        Label {
            id: lblLossy
            anchors.left: parent.left
            anchors.leftMargin: 6
            text: qsTr("Loss level:")
        }

        SpinBox {
            id: sbLossy
            enabled: cbLossy.checked
            anchors.left: parent.left
            anchors.leftMargin: 6
            maximumValue: 100
            value: __lossy_level_default__
            TooltipArea {
                id: tooltipArea
                text: qsTr("Loss factor (implies -lossy, default 100, but we use lesser value)")
            }
        }
    }
}
