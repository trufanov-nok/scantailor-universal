import QtQuick 2.0
import QtQuick.Controls 1.2

GroupBox {
    id: root
    title: qsTr("Required conversion:")
    width: mainApp.width

    property alias root: root
    property alias cbPostprocessors: cbPostprocessors
    //    property alias cbPostprocessorsHint: cbPostprocessorsHint
    property bool show_jpeg_settings: false
    property alias sbJpegQuality: sbJpegQuality
    property alias cbJpegGrayscale: cbJpegGrayscale
    property alias cbJpegSmooth: cbJpegSmooth
    property alias tooltipArea: tooltipArea
    property alias sbJpegSmooth: sbJpegSmooth

    //    TooltipArea {
    //        text: qsTr("Some DjVu encoders might not support Tiff images\n(which produced on Output processing stage). Thus\ntheir conversion to some temporary format might be\nrequired. Temporary image files will be deleted\nautomatically.")
    //    }
    Column {
        id: column
        width: parent.width

        ComboBox {
            id: cbPostprocessors
            currentIndex: 0
            textRole: "text"
            editable: false
            width: parent.width
            //            TooltipArea {
            //                id: cbPostprocessorsHint
            //            }
        }

        Label {
            id: lblJpegQuality
            text: qsTr("Jpeg quality:")
            visible: show_jpeg_settings
            TooltipArea {
                id: lblJpegQualityHint
                text: qsTr("Quality of the Jpeg image. As we use temporary Jpeg file just to pass an image to DjVu encoder\nit has sense to use 100 (max). But there still may be information loss\nin subsampling, as well as roundoff error.")
            }
        }

        SpinBox {
            id: sbJpegQuality
            visible: show_jpeg_settings
            value: 100
            maximumValue: 100
            TooltipArea {
                text: lblJpegQualityHint.text
            }
        }

        CheckBox {
            id: cbJpegGrayscale
            text: qsTr("Turn Jpeg to grayscale")
            visible: show_jpeg_settings
            tooltip: qsTr("Turns original image to grayscale during conversion to Jpeg.")
        }

        CheckBox {
            id: cbJpegSmooth
            checked: false
            text: qsTr("Smooth Jpeg image:")
            visible: show_jpeg_settings
            tooltip: qsTr("Filters the input to eliminate fine-scale noise.\nThis is often useful when converting dithered images.\nA moderate smoothing factor of 10 to 50 gets rid of dithering patterns.")
        }

        SpinBox {
            id: sbJpegSmooth
            visible: show_jpeg_settings
            enabled: cbJpegSmooth.checked
            value: 10
            maximumValue: 100
            TooltipArea {
                id: tooltipArea
                text: cbJpegSmooth.tooltip
            }
        }
    }
}
