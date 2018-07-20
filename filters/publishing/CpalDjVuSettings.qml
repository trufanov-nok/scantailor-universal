import QtQuick 2.4

CpalDjVuSettingsForm {
    property string type: "encoder"
    property string name: qsTr("cpaldjvu [DjVuLibre] for few colors")

    property string supportedInput: "jpeg;ppm;pgm;pbm"
    property string prefferedInput: "ppm"
    property string supportedOutputMode: "grayscale;color"
    property string description: qsTr("Encoder for images containing few colors. It performs best on images containing large solid color areas. This program works by first reducing the number of distinct colors to a small specified value using a simple color quantization algorithm. The dominant color is encoded into the background layer. The other colors are encoded into the foreground layer.")
    property int priority: 20



    property string current_platform: "linux"
    property string colors_val: "-colors"
    property string bgwhite_used: "-bgwhite"

    function init(platform) {
        current_platform = platform;
        if ("linux" === platform) {
            return true;
        } else return false;
    }

    function getDependencies() {
        return {"app":"cpaldjvu", "check_cmd":"%1 --help", "search_params":"-dpi;"+colors_val+";"+bgwhite_used}
    }

    function getMissingAppHint() {
        if (current_platform == "linux") {
          return qsTr("cpaldjvu is a part of djvulibre-bin package. Please install djvulibre-bin in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        if (cbColors.checked) {
            map[colors_val] = sbColors.value;
        }
        map[bgwhite_used] = cbBgwhite.checked;

        return map;
    }

    function setState(map) {
        cbColors.checked = map.contains(colors_val);
        sbColors.value = (cbColors.checked ? map[colors_val] : 256);
        cbBgwhite.checked = map.contains(bgwhite_used);
    }

    function getCommand(map) {
        s = "cpaldjvu -dpi " + map["_dpi_"]
        if (map.contains(colors_val)) {
           s += " " + colors_val + " " + map[colors_val]
        }
        if (map.contains(bgwhite_used)) {
            s += " " + bgwhite_used
        }
        return s
    }

}
