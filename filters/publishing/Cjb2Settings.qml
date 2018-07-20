import QtQuick 2.4

Cjb2SettingsForm {
    property string type: "encoder"
    property string name: qsTr("cjb2 [DjVuLibre] for b/w")
    property string supportedInput: "tiff;ppm;pgm;pbm"
    property string prefferedInput: "pbm"
    property string supportedOutputMode: "bw"
    property string description: qsTr("Simple encoder for bitonal files. Program produces a DjVuBitonal file. The default compression process is lossless: decoding the DjVuBitonal file at full resolution will  produce an image exactly identical to the input file. Lossy compression is enabled by options.")
    property int priority: 10

    
    
    property string current_platform: "linux"    
    property string param_clean: "-clean"
    property string param_lossy: "-lossy"
    property string param_losslevel: "-losslevel"

    function init(platform) {
        current_platform = platform;
        if ("linux" === platform) {
            return true;
        } else return false;
    }

    function getDependencies() {
        return {"app":"cjb2", "check_cmd":"%1", "search_params":"-dpi;"+ param_clean + ";" + param_lossy + ";" + param_losslevel}
    }

    function getMissingAppHint() {
        if (current_platform == "linux") {
          return qsTr("cjb2 is a part of djvulibre-bin package. Please install djvulibre-bin in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        map[param_clean] = cbClean.checked;
        map[param_lossy] = cbLossy.checked;
        if (sbLossy.enabled) {
            map[param_losslevel] = sbLossy.value;
        }

        return map;
    }

    function setState(map) {
        cbClean.checked = map[param_clean];
        cbLossy.checked = map[param_lossy];
        sbLossy.value = (map.contains(param_losslevel) ? map[param_losslevel] : 100);
    }

    function getCommand(map) {
        s = "cjb2 -dpi " + map["_dpi_"]
        if (map[param_clean]) {
           s += " " + param_clean;
        }
        if (map[param_lossy]) {
           s += " " + param_lossy;
        }
        if (map.contains(param_losslevel)) {
            s += " " + param_losslevel + " " + map[param_losslevel];
        }

        return s
    }

    cbClean.onCheckedChanged: {
        if (cbClean.checked == false) {
            cbLossy.checked = false;
        }
    }

}
