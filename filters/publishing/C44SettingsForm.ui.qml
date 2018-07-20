import QtQuick 2.0
import QtQuick.Controls 1.2

GroupBox {
    id: root
    title: qsTr("c44 encoder:")

    property alias cbGamma: cbGamma
    property alias sbGamma: sbGamma
    property alias cbChrominance: cbChrominance
    property alias cbChrominanceDelay: cbChrominanceDelay
    property alias sbChrominanceDelay: sbChrominanceDelay
    property alias tfSlice: tfSlice
    property alias tfBpp: tfBpp
    property alias tfSize: tfSize
    property alias tfPercent: tfPercent
    property alias tfDecibel: tfDecibel
    property alias cbDecibelFrac: cbDecibelFrac
    property alias sbDecibelFrac: sbDecibelFrac

    Column {
        id: column
        anchors.leftMargin: 6
        anchors.fill: parent

        CheckBox {
            id: cbGamma
            text: qsTr("Gamma:")
            tooltip: qsTr("Specify the gamma correction information encoded\ninto the output file. The argument n specified\nthe gamma value of the device for which the input\nimage was designed. The default value is 2.2.\nThis is appropriate for images designed for\na standard computer monitor.")
        }

        SpinBox {
            id: sbGamma
            decimals: 1
            stepSize: 0.1
            enabled: cbGamma.checked
            maximumValue: 4.9
            minimumValue: 0.3
            value: 2.2
            TooltipArea {
                text: cbGamma.tooltip
            }
        }

        Label {
            id: lblChrominance
            text: qsTr("Chrominance encoding:")
        }

        ComboBox {
            id: cbChrominance
            width: parent.width
            textRole: "txt"
            model: ListModel {
                ListElement {
                    txt: qsTr("Normal")
                    tooltip: qsTr("Select normal chrominance encoding.\nChrominance information is encoded at\nthe same resolution as the luminance.\nThis is the default.")
                }
                ListElement {
                    txt: qsTr("Half")
                    tooltip: qsTr("Selects half resolution chrominance encoding.\nChrominance information is encoded at\nhalf the luminance resolution.")
                }
                ListElement {
                    txt: qsTr("Full")
                    tooltip: qsTr("Select the highest possible quality for\nencoding the chrominance information.\nThis is equivalent to selection Normal and\nDelay = 0.")
                }
                ListElement {
                    txt: qsTr("None")
                    tooltip: qsTr("Disable the encoding of the chrominance.\nOnly the luminance information will be encoded.\nThe resulting image will show in shades of gray.")
                }
            }
            currentIndex: 0
            editable: false
        }

        CheckBox {
            id: cbChrominanceDelay
            text: qsTr("Chrominance coding delay:")
            visible: cbChrominance.currentIndex < 2
        }

        SpinBox {
            id: sbChrominanceDelay
            maximumValue: 99
            minimumValue: 0
            value: 10
            visible: cbChrominanceDelay.visible
            enabled: cbChrominanceDelay.checked
        }

        Label {
            text: qsTr("Quality selection options:")
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        Label {
            text: qsTr("Slice:")
            TooltipArea {
                text: taSlice.text
            }
        }

        TextField {
            id: tfSlice
            placeholderText: "n+...+n"
            width: parent.width
            validator: RegExpValidator {
                regExp: /\d{1,3}(?:+\d{1,3})+$/
            }
            TooltipArea {
                id: taSlice
                text: qsTr("Specify the number of slices (from 1 to 140)\nin each chunk. The option argument contains\nplus-separated numerical values (one per chunk)\nindicating the number of slices per chunk.\nOption 74+13+10, for instance, would be appropriate\nfor compressing a photographic image with three\nprogressive refinements. More quality and more\nrefinements can be obtained with option 72+11+10+10.\nDefault value: 74,89,99")
            }
        }

        Label {
            text: qsTr("Bpp:")
            TooltipArea {
                text: taBpp.text
            }
        }

        TextField {
            id: tfBpp
            placeholderText: "n,...,n"
            width: parent.width
            validator: RegExpValidator {
                regExp: /(\d{1,3})(\.\d{1,2})?(?:[,+](\d{1,3})(\.\d{1,2})?)+$/
            }
            TooltipArea {
                id: taBpp
                text: qsTr("Specify size targets for each chunk expressed\nin bits-per-pixel. Both comma-separated and\nplus-separated specifications are accepted.\nOption 0.25,0.5,1 usually provides good results.")
            }
        }

        Label {
            text: qsTr("Size:")
            TooltipArea {
                text: taSize.text
            }
        }

        TextField {
            id: tfSize
            placeholderText: "n,...,n"
            width: parent.width
            validator: RegExpValidator {
                regExp: /\d{1,10}(?:[,+]\d{1,10})+$/
            }
            TooltipArea {
                id: taSize
                text: qsTr("Specify size targets for each chunk expressed in bytes.\nThe option argument can be either a plus-separated\nlist specifying a size for each chunk, or a comma separated\nlist specifying cumulative sizes for each chunk and all\nprevious chunks. Size targets are approximates. Slices\nwill be added to each chunk until exceeding\nthe specified target.")
            }
        }

        Label {
            text: qsTr("Percent:")
            TooltipArea {
                text: taPercent.text
            }
        }

        TextField {
            id: tfPercent
            placeholderText: "n,...,n"
            width: parent.width
            validator: RegExpValidator {
                regExp: /\d{1,3}(?:[,+]\d{1,3})+$/
            }
            TooltipArea {
                id: taPercent
                text: qsTr("Specify size targets for each chunk expressed as\na percentage of the input file size.\nBoth comma-separated and plus-separated specifications\nare accepted. Results can be drastically different\naccording to the format of the input image\n(raw or JPEG compressed).")
            }
        }

        Label {
            text: qsTr("Decibel:")
            TooltipArea {
                text: taDecibel.text
            }
        }

        TextField {
            id: tfDecibel
            placeholderText: "n,...,n"
            width: parent.width
            validator: RegExpValidator {
                regExp: /\d{2}(?:,\d{2})+$/
            }
            TooltipArea {
                id: taDecibel
                text: qsTr("Specify quality targets for each chunk expressed\nas a comma-separated list of increasing decibel values.\nDecibel values range from 16 (very low quality)\nto 48 (very high quality). This criterion should not\nbe relied upon when re-encoding an image previously\ncompressed by another compression scheme. Selecting\nthis option significantly increases the compression time..")
            }
        }

        CheckBox {
            text: qsTr("Decibel frac:")
            id: cbDecibelFrac
            checked: false
            visible: tfDecibel.text != ""
            tooltip: qsTr("Indicate that the decibel values specified in\noption Decibel should be computed by averaging\nthe mean squared errors of only the fraction\nfrac of the most mis-represented blocks\nof 32 x 32 pixels. This option is useful with\ncomposite images containing solid color features\n(e.g. an image with a large white border).")
        }

        SpinBox {
            id: sbDecibelFrac
            suffix: "%"
            decimals: 1
            maximumValue: 100
            visible: cbDecibelFrac.visible
            enabled: cbDecibelFrac.checked
            TooltipArea {
                id: taDecibelFrac
                text: cbDecibelFrac.tooltip
            }
        }
    }
}
