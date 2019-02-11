import QtQuick 2.0

TiffPostprocessorsForm {

    property string type: "converter"

    property string current_platform: "linux"

    property string param_quality: "--quality"
    property string param_grayscale: "--grayscale"
    property string param_smooth: "--smooth"


    root.onHeightChanged: {
        mainApp.resizeQuickWidget(type);
    }

    signal notify()

    property bool block_notify: false;

    function blockNotify() { block_notify = true; }
    function unblockNotify() { block_notify = false; }

    onNotify: {
        if (!block_notify) {
            mainApp.requestParamUpdate(type);
        }
    }

    Component.onCompleted: {
        sbJpegQuality.valueChanged.connect(notify);
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
            el_id: "no_conv"
        }
        ListElement {
            text: qsTr("PPM")
            dependency: "tifftopnm"
            format: "ppm"
            el_id: "tifftopnm"
        }
        ListElement {
            text: qsTr("PGM (grayscale)")
            dependency: "tifftopnm,ppmtopgm"
            format: "pgm"
            el_id: "ppmtopgm"
        }
        ListElement {
            text: qsTr("PBM (black and white)")
            dependency: "tifftopnm,ppmtopgm,pgmtopbm"
            format: "pbm"
            el_id: "pgmtopbm"
        }
        ListElement {
            text: qsTr("JPEG")
            dependency: "tifftopnm,pnmtojpeg"
            format: "jpeg"
            el_id: "pnmtojpeg"
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
        var apps = ["tifftopnm", "ppmtopgm", "pgmtopbm", "pnmtojpeg"]

        var res = [];
        for (var i in apps) {
            res.push({"app":apps[i], "check_cmd":"%1 --help", "search_params":"", "missing_app_hint":getMissingAppHint(apps[i])});
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

        cbPostprocessors.menu = null;
        cbPostprocessors.model = filtered_model;
        cbPostprocessors.textRole = "text";

        if (filtered_model.count > 0) {            
            if (cbPostprocessors.currentIndex != pref_idx) {
                cbPostprocessors.currentIndex = pref_idx;
            } else {
                cbPostprocessors.currentIndex = -1;
                cbPostprocessors.currentIndex = pref_idx;
                //cbPostprocessors.currentIndexChanged();
            }
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
        var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
        state["el_id"] = el.el_id;

        if (sbJpegQuality.visible) {
            state[param_quality] = sbJpegQuality.value;
        }

        if (cbJpegGrayscale.visible && cbJpegGrayscale.checked) {
            state[param_grayscale] = cbJpegGrayscale.checked;
        }

        if (cbJpegSmooth.visible && cbJpegSmooth.checked) {
            state[param_smooth] = sbJpegSmooth.value;
        }

        return state;
    }

    function setState(state) {

        blockNotify();

        if ("el_id" in state) {
            for (var i = 0; i < cbPostprocessors.model.count; ++i) {
                var el = cbPostprocessors.model.get(i);
                if (el.el_id === state["el_id"]) {
                    if (cbPostprocessors.currentIndex != i) {
                        cbPostprocessors.currentIndex = i;
                    }
                    break;
                }
            }
        }

        sbJpegQuality.value = (param_quality in state ? state[param_quality] : 100);
        cbJpegGrayscale.checked = (param_grayscale in state ? state[param_grayscale] : false);
        cbJpegSmooth.checked = param_smooth in state;
        sbJpegSmooth.value = (cbJpegSmooth.checked ? state[param_smooth] : 0);

        unblockNotify();
    }

    function getCommandFromState(state) {
        var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
        if (el.format === "tiff") {
            return "";
        } else if (el.format === "ppm") {
            return "tifftopnm %1 > %1";
        } else if (el.format === "pgm") {
            return "tifftopnm %1 | ppmtopgm > %1";
        } else if (el.format === "pbm") {
            return "tifftopnm %1 | ppmtopgm | pgmtopbm > %1";
        } else if (el.format === "jpeg") {
            var s = "tifftopnm %1 | pnmtojpeg ";
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
        if (cbPostprocessors.currentIndex != -1) {
            // cbPostprocessorsHint.text = getDescription();
            var el = cbPostprocessors.model.get(cbPostprocessors.currentIndex);
            show_jpeg_settings = (el.format === "jpeg");
        }
    }


}
