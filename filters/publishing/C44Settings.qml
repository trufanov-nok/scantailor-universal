import QtQuick 2.4

C44SettingsForm {

    property string type: "encoder"
    property string name: qsTr("C44 [DjVuLibre] for color")

    property string supportedInput: "jpeg;ppm;pgm;pbm"
    property string prefferedInput: "ppm"
    property string supportedOutputMode: "color"
    property string description: qsTr("Produces a DjVuPhoto encoded image.")
    property int priority: 30
    property int _dpi_: 600

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


    signal notify()

    onNotify: {
        mainApp.requestParamUpdate(type);
    }

    Component.onCompleted: {
        var cbs = [cbDecibelFrac, cbChrominanceDelay, cbGamma];
        for (var i in cbs) {
            cbs[i].checkedChanged.connect(notify);
        }
        var sbs = [sbDecibelFrac, sbChrominanceDelay, sbGamma];
        for (i in sbs) {
            sbs[i].valueChanged.connect(notify);
        }
        var tes = [tfDecibel, tfPercent, tfSize, tfBpp, tfSlice];
        for (i in tes) {
            tes[i].textChanged.connect(notify);
        }
        cbChrominance.currentIndexChanged.connect(notify);
    }

    function init(platform) {
        current_platform = platform;

        if ("linux" === platform) {
            return true;
        } else return false;


    }

    function getDependencies() {
        return {"app":"c44", "check_cmd":"%1", "search_params":"-dpi;"+ param_slice + ";" + param_bpp + ";" + param_size + ";" +
                                                               param_percent + ";" + param_decibel + ";" + param_dbfrac + ";" + param_mask + ";" + param_gamma + ";" + param_crcbfull + ";" + param_crcbnormal + ";" +
                                                               param_crcbhalf + ";" + param_crcbnone + ";" + param_crcbdelay,
            "missing_app_hint": getMissingAppHint() }
    }

    function getMissingAppHint() {
        if (current_platform == "linux") {
            return qsTr("c44 is a part of djvulibre-bin package. Please install djvulibre-bin in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        var state = {"_dpi_":_dpi_};
        if (cbGamma.checked) {
            state[param_gamma] = sbGamma.value;
        }

        state[chrominances_setting] = chrominances[cbChrominance.currentIndex];

        if (cbChrominanceDelay.enabled && cbChrominanceDelay.checked) {
            state[param_crcbdelay] = sbChrominanceDelay.value;
        }

        if (tfSlice.text !== "") {
            state[param_slice] = tfSlice.text;
        }

        if (tfBpp.text !== "") {
            state[param_bpp] = tfBpp.text;
        }

        if (tfSize.text !== "") {
            state[param_size] = tfSize.text;
        }

        if (tfPercent.text !== "") {
            state[param_percent] = tfPercent.text;
        }

        if (tfDecibel.text !== "") {
            state[param_decibel] = tfDecibel.text;
        }

        if (cbDecibelFrac.checked) {
            state[param_dbfrac] = sbDecibelFrac.value;
        }

        return state;
    }


    function setState(state) {
        _dpi_ = state["_dpi_"];
        cbGamma.checked = param_gamma in state;
        sbGamma.value = (cbGamma.checked ? state[param_gamma] : 0);

        cbChrominance.currentIndex = chrominances.indexOf(state[chrominances_setting]);

        cbChrominanceDelay.checked = param_crcbdelay in state;
        sbChrominanceDelay.value = (cbChrominanceDelay.checked ? state[param_crcbdelay] : 10);

        tfSlice.text = param_slice in state ? state[param_slice] : "";
        tfBpp.text = param_bpp in state ? state[param_bpp] : "";
        tfSize.text = param_size in state ? state[param_size] : "";
        tfPercent.text = param_percent in state ? state[param_percent] : "";
        tfDecibel.text = param_decibel in state ? state[param_decibel] : "";
        cbDecibelFrac.checked = param_dbfrac in state;
        sbDecibelFrac.value = (cbDecibelFrac.checked ? state[param_dbfrac] : 0);

        notify();
    }

    function insert_param(str, state, param) {
        if (param in state) {
            str += " " + param + " " + state[param];
        }
        return str;
    }

    function getCommandFromState(state) {
        var s = "c44 -dpi " + state["_dpi_"];

        s = insert_param(s, state, param_gamma);

        s += " " + state[chrominances_setting];

        s = insert_param(s, state, param_crcbdelay);
        s = insert_param(s, state, param_slice);
        s = insert_param(s, state, param_bpp);
        s = insert_param(s, state, param_size);
        s = insert_param(s, state, param_percent);
        s = insert_param(s, state, param_decibel);
        s = insert_param(s, state, param_dbfrac);

        s += " %1 %2";
        return s;
    }

    function getCommand() {
        return getCommandFromState(getState());
    }

    root.onHeightChanged: {
        mainApp.resizeQuickWidget(type);
    }
}
