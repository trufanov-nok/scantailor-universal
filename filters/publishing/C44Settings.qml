import QtQuick 2.4

C44SettingsForm {

    property string type: "encoder"
    property string name: qsTr("C44 [DjVuLibre] for color")

    property string supportedInput: "jpeg;ppm;pgm;pbm"
    property string prefferedInput: "ppm"
    property string supportedOutputMode: "color"
    property string description: qsTr("Produces a DjVuPhoto encoded image.")
    property int priority: 30

    property string current_platform: "linux"

    property string param_slice: "-slice"
    property string param_bpp: "-bpp"
    property string param_size: "-size"
    property string param_percent: "-percent"
    property string param_decibel: "-decibel"
    property string param_dbfrac: "-dbfrac"
    property string param_mask: "-mask"
//    property string param_dpi: "-dpi"
    property string param_gamma: "-gamma"
    property string param_crcbfull: "-crcbfull"
    property string param_crcbnormal: "-crcbnormal"
    property string param_crcbhalf: "-crcbhalf"
    property string param_crcbnone: "-crcbnone"
    property string param_crcbdelay: "-crcbdelay"
    property variant chrominances: [param_crcbnormal, param_crcbhalf, param_crcbfull, param_crcbnone]
    property string chrominances_setting: "chrominance"

    function init(platform) {
        current_platform = platform;
        if ("linux" === platform) {
            return true;
        } else return false;
    }

    function getDependencies() {
        return {"app":"c44", "check_cmd":"%1", "search_params":"-dpi;"+ param_slice + ";" + param_bpp + ";" + param_size + ";" +
            param_percent + ";" + param_decibel + ";" + param_dbfrac + ";" + param_mask + ";" + param_gamma + ";" + param_crcbfull + ";" + param_crcbnormal + ";" +
            param_crcbhalf + ";" + param_crcbnone + ";" + param_crcbdelay}
    }

    function getMissingAppHint() {
        if (current_platform == "linux") {
          return qsTr("c44 is a part of djvulibre-bin package. Please install djvulibre-bin in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        if (cbGamma.checked) {
            map[param_gamma] = sbGamma.value;
        }

        map[chrominances_setting] = chrominances[cbChrominance.currentIndex];

        if (cbChrominanceDelay.enabled && cbChrominanceDelay.checked) {
            map[param_crcbdelay] = sbChrominanceDelay.value;
        }

        map[bgwhite_used] = cbBgwhite.checked;

        if (tfSlice.text !== "") {
            map[param_slice] = tfSlice.text;
        }

        if (tfBpp.text !== "") {
            map[param_bpp] = tfBpp.text;
        }

        if (tfSize.text !== "") {
            map[param_size] = tfSize.text;
        }

        if (tfPercent.text !== "") {
            map[param_percent] = tfPercent.text;
        }

        if (tfDecibel.text !== "") {
            map[param_decibel] = tfDecibel.text;
        }

        if (cbDecibelFrac.checked) {
            map[param_dbfrac] = sbDecibelFrac.value;
        }

        return map;
    }

    function setState(map) {
        cbGamma.checked = map.contains(param_gamma);
        sbGamma.value = (cbGamma.checked ? map[param_gamma] : 0);

        cbChrominance.currentIndex = chrominances.indexOf(map[chrominances_setting]);

        cbChrominanceDelay.checked = map.contains(param_crcbdelay);
        sbChrominanceDelay.value = (cbChrominanceDelay.checked ? map[param_crcbdelay] : 10);

        tfSlice.text = map.contains(param_slice) ? map[param_slice] : "";
        tfBpp.text = map.contains(param_bpp) ? map[param_bpp] : "";
        tfSize.text = map.contains(param_size) ? map[param_size] : "";
        tfPercent.text = map.contains(param_percent) ? map[param_percent] : "";
        tfDecibel.text = map.contains(param_decibel) ? map[param_decibel] : "";
        cbDecibelFrac.checked = map.contains(param_dbfrac);
        sbDecibelFrac.value = (cbDecibelFrac.checked ? map[param_dbfrac] : 0);
    }

    function insert_param(str, map, param) {
        if (map.contains(param)) {
            str += " " + param + " " + map[param];
        }
        return str;
    }

    function getCommand(map) {
        s = "c44 -dpi " + map["_dpi_"];

        s = insert_param(s, map, param_gamma);

        s += " " + map[chrominances_setting];

        s = insert_param(s, map, param_crcbdelay);
        s = insert_param(s, map, param_slice);
        s = insert_param(s, map, param_bpp);
        s = insert_param(s, map, param_size);
        s = insert_param(s, map, param_percent);
        s = insert_param(s, map, param_decibel);
        s = insert_param(s, map, param_dbfrac);
        return s;
    }
}
