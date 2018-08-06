import QtQuick 2.0

TiffPostprocessorsForm {

    property string type: "convertor"

    property string current_platform: "linux"

    property string param_quality: "--quality"
    property string param_grayscale: "--grayscale"
    property string param_smooth: "--smooth"


    root.onHeightChanged: {
        mainApp.resizeQuickWidget(type);
    }

    signal notify()

    onNotify: {
        mainApp.requestParamUpdate(type);
    }

    Component.onCompleted: {
        cbJpegSmooth.checkedChanged.connect(notify);
        sbJpegSmooth.valueChanged.connect(notify);
        cbJpegGrayscale.checkedChanged.connect(notify);
        cbPostprocessors.currentIndexChanged.connect(notify);
    }


    ListModel {

        id: model

        ListElement {
            text: qsTr("No conversion (use Tiff)")
            dependency: ""
            format: "tiff"
        }
        ListElement {
            text: qsTr("PPM")
            dependency: "tifftopnm"
            format: "ppm"
        }
        ListElement {
            text: qsTr("PGM (grayscale)")
            dependency: "tifftopnm,ppmtopgm"
            format: "pgm"
        }
        ListElement {
            text: qsTr("PBM (black and white)")
            dependency: "tifftopnm,ppmtopgm,pgmtopbm"
            format: "pbm"
        }
        ListElement {
            text: qsTr("JPEG")
            dependency: "tifftopnm,pnmtojpeg"
            format: "jpeg"
        }
    }


    function init(platform) {
        current_platform = platform;
        if (current_platform === "linux") {
            cbPostprocessors.model = model;
            cbPostprocessors.textRole = "text";
            return true;
        }
        return false;
    }

    function getDependencies() {
        var apps = ["tifftopnm","ppmtopgm","pgmtopbm","pnmtojpeg"]

        var res = [];
        for (var i in apps) {
            res.push({"app":apps[i], "check_cmd":"%1 --help", "search_params":"","missing_app_hint": getMissingAppHint(apps[i])});
        }

        return res
    }

    ListModel {
        id: filtered_model
    }

    function filterByRequiredInput(formats, preffered_format) {
        filtered_model.clear();

        var pref_idx = 0;
        for (var i = 0; i < model.count; ++i) {
            var el = model.get(i);
            if (formats.indexOf(el.format) >= 0) {
                if (el.format === preffered_format) {
                    pref_idx = filtered_model.count;
                }
                filtered_model.append(el);
            }
        }

        cbPostprocessors.model = filtered_model;
        cbPostprocessors.textRole = "text";
        if (filtered_model.count > 0) {
            cbPostprocessors.currentIndex = pref_idx;
        }
    }


    function getDescription() {
        var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
        var f = el.format;
        if (f === "tiff") {
            return qsTr("No image format conversion needed as djvu encoder supports tiff images.");
        } else
            if (current_platform == "linux") {
                if (f === "ppm") {
                    return qsTr("Convert Tiff image to PPM before encoding to DjVu with help of tifftopnm. PPM could contain color image but in fact PGM or even PBM might be generated depending on original Tiff format");
                } else if (f === "pgm") {
                    return qsTr("Convert Tiff image to PGM before encoding to DjVu with help of tifftopnm and pgmtopbm. PGM is always grayscale so image will be turned to grayscale if needed.");
                } else if (f === "pbm") {
                    return qsTr("Convert Tiff image to PBM before encoding to DjVu with help of tifftopnm, ppmtopgm and pgmtopbm. PBM is always black and white so image will be binarized if needed.");
                } else if (f === "jpeg") {
                    return qsTr("Convert Tiff image to JPEG before encoding to DjVu with help of tifftopnm and pnmtojpeg.");
                }
            }
        return "";

    }



    function getMissingAppHint(app) {
        if (current_platform == "linux") {
            return qsTr(app + " is a part of netpbm package. Please install netpbm in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        var state = {};

        state[param_quality] = sbJpegQuality.value;
        state[param_grayscale] = cbJpegGrayscale.checked;
        if (cbJpegSmooth.checked) {
            state[param_smooth] = sbJpegSmooth.value;
        }

        return state;
    }

    function setState(state) {
        sbJpegQuality.value = state[param_quality];
        cbJpegGrayscale.checked = state[param_grayscale];
        cbJpegSmooth.checked = param_smooth in state;
        sbJpegSmooth.value = (cbJpegSmooth.checked ? state[param_smooth] : 0);
        notify();
    }

    function getCommandFromState(state) {
        var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
        if (el.format === "tiff") {
            return "";
        } else if (el.format === "ppm") {
            return "tifftopnm %1 > %1";
        } else if (el.format === "pgm") {
            return "tifftopnm %1 > ppmtopgm > %1";
        } else if (el.format === "pbm") {
            return "tifftopnm %1 > ppmtopgm > pgmtopbm > %1";
        } else if (el.format === "jpeg") {
            var s = "tifftopnm %1 > pnmtojpeg ";
            s += param_quality + " " + state[param_quality]+ " ";
            if (param_grayscale in state) {
                s += param_grayscale + " ";
            }
            if (param_smooth in state) {
                s += param_smooth + " " + state[param_smooth]+ " ";
            }
            s += " > %1";
            return s;
        }

        return "";
    }

    function getCommand() {
        return getCommandFromState(getState());
    }

    cbPostprocessors.onCurrentIndexChanged: {
        // cbPostprocessorsHint.text = getDescription();
        var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
        show_jpeg_settings = (el.format === "jpeg");
    }


}
