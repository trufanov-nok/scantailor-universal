import QtQuick 2.4
import QtQuick.Controls 1.2

GroupBox {
    id: root
    title: qsTr("Required convertion:")
    property alias cbPostprocessors: cbPostprocessors
    property alias cbPostprocessorsHint: cbPostprocessorsHint

    Column {
        id: column
        spacing: 2

        ComboBox {
            id: cbPostprocessors
            width: parent.width
            currentIndex: 0
            editable: false
            TooltipArea {
                id: cbPostprocessorsHint
            }
        }

        GroupBox {
            id: groupBox
            width: root.width - 8 * 2
            Column {
                id: column1

                Label {
                    id: lblJpegQuality
                    text: qsTr("Jpeg quality:")
                    TooltipArea {
                        id: lblJpegQualityHint
                        text: qsTr("Quality of the Jpeg image. As we use temporary Jpeg file just to pass an image to DjVu encoder\nit has sense to use 100 (max). But there still may be information loss\nin subsampling, as well as roundoff error.")
                    }
                }

                SpinBox {
                    id: sbJpegQuality
                    value: 100
                    maximumValue: 100
                    TooltipArea {
                        text: lblJpegQualityHint.text
                    }
                }

                CheckBox {
                    id: cbJpegGrayscale
                    text: qsTr("Turn Jpeg to grayscale")
                    tooltip: qsTr("Turns original image to grayscale during conversion to Jpeg.")
                }

                CheckBox {
                    id: cbJpegSmooth
                    checked: false
                    text: qsTr("Smooth Jpeg image:")
                    tooltip: qsTr("Filters the input to eliminate fine-scale noise.\nThis is often useful when converting dithered images.\nA moderate smoothing factor of 10 to 50 gets rid of dithering patterns.")
                }

                SpinBox {
                    id: sbJpegSmooth
                    enabled: cbJpegSmooth.checked
                    value: 10
                    maximumValue: 100
                    TooltipArea {
                        text: cbJpegSmooth.tooltip
                    }
                }

                TextArea {
                    id: taHint
                    backgroundVisible: false
                    readOnly: true
                    width: root.width - 8 * 2 * 2

                    text: qsTr("Some DjVu encoders might not support Tiff images (which produced on Output processing stage). Thus their conversion to some temporary format might be required. Temporary image files will be deleted automatically.")
                }
            }
        }
    }
}
