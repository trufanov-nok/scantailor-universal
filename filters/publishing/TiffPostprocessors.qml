import QtQuick 2.4

TiffPostprocessorsForm {

//    property string current_platform: "linux"


//    ListModel {

//        id: model

//        ListElement {
//            text: qsTr("No conversion (use Tiff)")
//            dependency: ""
//            format: "tiff"
//        }
//        ListElement {
//            text: qsTr("PPM")
//            dependency: "tifftopnm"
//            format: "ppm"
//        }
//        ListElement {
//            text: qsTr("PGM (grayscale)")
//            dependency: "tifftopnm,ppmtopgm"
//            format: "pgm"
//        }
//        ListElement {
//            text: qsTr("PBM (black and white)")
//            dependency: "tifftopnm,ppmtopgm,pgmtopbm"
//            format: "pbm"
//        }
//        ListElement {
//            text: qsTr("JPEG")
//            dependency: "tifftopnm,pnmtojpeg"
//            format: "jpeg"
//        }
//    }


//    function init(platform) {
//        current_platform = platform;
//        cbPostprocessors.model = model;
//        cbPostprocessors.textRole = text;
//    }

//    function getDependency() {
//        idx = cbPostprocessors.currentIndex;
//        app = cbPostprocessors.model[idx].dependency;
//        return {"app":app, "check_cmd": app!=""? "%1 --help" : "", "search_params":""}
//    }

//    ListModel {
//        id: filtered_model
//    }

//    function filterByRequiredInput(formats) {
//        filtered_model.clear();

//        for (el in model) {
//            if (formats.indexOf(el.format) >= 0) {
//                filtered_model.append(l);
//            }
//        }

//        cbPostprocessors.model = filtered_model;
//        cbPostprocessors.textRole = text;
//        if (filtered_model.count > 0) {
//            cbPostprocessors.currentIndex = 0;
//        }
//    }


//    function getDescription() {

//        var f = cbPostprocessors.model[cbPostprocessors.currentIndex].format;
//        if (f === "tiff") {
//            return qsTr("No image format conversion needed as djvu encoder supports tiff images.");
//        } else
//            if (current_platform == "linux") {
//                if (f === "ppm") {
//                    return qsTr("Convert Tiff image to PPM before encoding to DjVu with help of tifftopnm. PPM could contain color image but in fact PGM or even PBM might be generated depending on original Tiff format");
//                } else if (f === "pgm") {
//                    return qsTr("Convert Tiff image to PGM before encoding to DjVu with help of tifftopnm and pgmtopbm. PGM is always grayscale so image will be turned to grayscale if needed.");
//                } else if (f === "pbm") {
//                    return qsTr("Convert Tiff image to PBM before encoding to DjVu with help of tifftopnm, ppmtopgm and pgmtopbm. PBM is always black and white so image will be binarized if needed.");
//                } else if (f === "jpeg") {
//                    return qsTr("Convert Tiff image to JPEG before encoding to DjVu with help of tifftopnm and pnmtojpeg.");
//                }
//            }
//        return "";

//    }



//    function getMissingAppHint(app) {
//        if (current_platform == "linux") {
//          return qsTr(app + " is a part of netpbm package. Please install netpbm in case this appplication is missing in your system.");
//        } else return "";
//    }

//    function getState() {
//        map[param_clean] = cbClean.checked;
//        map[param_lossy] = cbLossy.checked;
//        if (sbLossy.enabled) {
//            map[param_losslevel] = sbLossy.value;
//        }

//        return map;
//    }

//    function setState(map) {
//        cbClean.checked = map[param_clean];
//        cbLossy.checked = map[param_lossy];
//        sbLossy.value = (map.contains(param_losslevel) ? map[param_losslevel] : 100);
//    }

//    function getCommand(map) {
//        s = "cjb2 -dpi " + map["_dpi_"]
//        if (map[param_clean]) {
//           s += " " + param_clean;
//        }
//        if (map[param_lossy]) {
//           s += " " + param_lossy;
//        }
//        if (map.contains(param_losslevel)) {
//            s += " " + param_losslevel + " " + map[param_losslevel];
//        }

//        return s
//    }

//    cbPostprocessors.onCurrentIndexChanged: {
//        cbPostprocessorsHint.text = getDescription();
//        if (cbPostprocessors.model[cbPostprocessors.currentIndex].format === "jpeg") {

//        }
//    }
}
