import QtQuick 2.4

Cjb2SettingsForm {
    property string type: "encoder"
    property string name: qsTr("cjb2 [DjVuLibre] for b/w")
    property string id: "cjb2"
    property string supportedInput: "tiff;ppm;pgm;pbm"
    property string prefferedInput: "pbm"
    property string supportedOutputMode: "bw"
    property string description: qsTr("Simple encoder for bitonal files. Program produces a DjVuBitonal file. The default compression process is lossless: decoding the DjVuBitonal file at full resolution will  produce an image exactly identical to the input file. Lossy compression is enabled by options.")
    property int priority: 10
    property int _dpi_: 600

    
    
    property string current_platform: "linux"
    property string param_clean: "-clean"
    property string param_lossy: "-lossy"
    property string param_losslevel: "-losslevel"

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
        cbClean.checkedChanged.connect(notify);
        sbLossy.valueChanged.connect(notify);
        cbLossy.checkedChanged.connect(notify);
    }

    function init(platform) {
        current_platform = platform;
        if ("linux" === platform) {
            return true;
        } else return false;
    }

    function getDependencies() {
        return {"app":"cjb2", "check_cmd":"%1", "search_params":"-dpi;"+ param_clean + ";" + param_lossy + ";" + param_losslevel, "missing_app_hint": getMissingAppHint() }
    }

    function getMissingAppHint() {
        if (current_platform == "linux") {
            return qsTr("cjb2 is a part of djvulibre-bin package. Please install djvulibre-bin in case this appplication is missing in your system.");
        } else return "";
    }

    function getState() {
        var state = {"_dpi_":_dpi_};
        state[param_clean] = cbClean.checked;
        state[param_lossy] = cbLossy.checked;
        if (sbLossy.enabled) {
            state[param_losslevel] = sbLossy.value;
        }

        return state;
    }

    function setState(state) {
        blockNotify();
        _dpi_ = ("_dpi_" in state ? state["_dpi_"] : 600);
        cbClean.checked = (param_clean in state ? state[param_clean] : false);
        cbLossy.checked = (param_lossy in state ? state[param_lossy] : false);
        sbLossy.value = (param_losslevel in state ? state[param_losslevel] : 100);
        unblockNotify();
    }

    function getCommandFromState(state) {
        var s = "cjb2 -dpi " + state["_dpi_"]
        if (state[param_clean]) {
            s += " " + param_clean;
        }
        if (state[param_lossy]) {
            s += " " + param_lossy;
        }
        if (param_losslevel in state) {
            s += " " + param_losslevel + " " + state[param_losslevel];
        }

        s += " %1 %2";
        return s
    }

    function getCommand() {
        return getCommandFromState(getState());
    }

    cbClean.onCheckedChanged: {
        if (cbClean.checked == false) {
            cbLossy.checked = false;
        }
    }

    root.onHeightChanged: {
        mainApp.resizeQuickWidget(type);
    }

}
