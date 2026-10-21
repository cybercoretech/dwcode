import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine

ApplicationWindow {
    id: win
    width: 1440
    height: 900
    minimumWidth: 980
    minimumHeight: 620
    visible: true
    title: "DWCode — DarkwaveCode Editor"
    color: theme.background

    property bool explorerVisible: true
    property bool bottomVisible: true
    property bool splitVisible: false
    property bool modalMode: false
    property string activeBottomTab: "OUTPUT"
    property string selectedFile: "src/main.ts"
    property string paletteFilter: ""
    property var files: [
        { name: "src", kind: "folder", depth: 0 },
        { name: "main.ts", kind: "file", depth: 1, language: "typescript" },
        { name: "workspace.ts", kind: "file", depth: 1, language: "typescript" },
        { name: "theme.css", kind: "file", depth: 0, language: "css" },
        { name: "README.md", kind: "file", depth: 0, language: "markdown" }
    ]
    property var commands: [
        { title: "Toggle File Explorer", shortcut: "Ctrl+B", action: "explorer" },
        { title: "Toggle Bottom Panel", shortcut: "Ctrl+J", action: "panel" },
        { title: "Split Editor", shortcut: "Ctrl+\\", action: "split" },
        { title: "Toggle Helix Mode", shortcut: "", action: "modal" },
        { title: "Show Output Panel", shortcut: "", action: "output" },
        { title: "Show Problems Panel", shortcut: "", action: "problems" }
    ]

    Theme { id: theme }

    function activateCommand(action) {
        palette.visible = false
        if (action === "explorer") explorerVisible = !explorerVisible
        else if (action === "panel") bottomVisible = !bottomVisible
        else if (action === "split") splitVisible = !splitVisible
        else if (action === "modal") toggleModalMode()
        else if (action === "output") { activeBottomTab = "OUTPUT"; bottomVisible = true }
        else if (action === "problems") { activeBottomTab = "PROBLEMS"; bottomVisible = true }
    }

    function toggleModalMode() {
        modalMode = !modalMode
        editor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode(" + (modalMode ? "true" : "false") + ")")
        if (splitVisible)
            splitEditor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode(" + (modalMode ? "true" : "false") + ")")
    }

    function openFile(fileName) {
        selectedFile = fileName
        const encoded = JSON.stringify(fileName)
        editor.runJavaScript("window.dwcodeOpenSample && window.dwcodeOpenSample(" + encoded + ")")
        if (splitVisible)
            splitEditor.runJavaScript("window.dwcodeOpenSample && window.dwcodeOpenSample(" + encoded + ")")
    }

    Shortcut { sequence: "Ctrl+Shift+P"; context: Qt.ApplicationShortcut; onActivated: { palette.visible = true; commandInput.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+P"; context: Qt.ApplicationShortcut; onActivated: { palette.visible = true; commandInput.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+B"; context: Qt.ApplicationShortcut; onActivated: win.explorerVisible = !win.explorerVisible }
    Shortcut { sequence: "Ctrl+J"; context: Qt.ApplicationShortcut; onActivated: win.bottomVisible = !win.bottomVisible }
    Shortcut { sequence: "Ctrl+\\"; context: Qt.ApplicationShortcut; onActivated: win.splitVisible = !win.splitVisible }
    Shortcut { sequence: "Ctrl+Shift+M"; context: Qt.ApplicationShortcut; onActivated: win.toggleModalMode() }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; enabled: palette.visible; onActivated: palette.visible = false }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: theme.shell
            border.color: "#262e3a"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 14
                spacing: 12
                Image { Layout.preferredWidth: 27; Layout.preferredHeight: 27; source: "qrc:/qml/dwcode-mark.svg"; sourceSize.width: 54; sourceSize.height: 54; smooth: true }
                Column {
                    spacing: 1
                    Text { text: "DWCODE"; color: "#e7ecf3"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.3 }
                    Text { text: "DARKWAVE CODE EDITOR"; color: "#778392"; font.pixelSize: 8; font.family: "monospace"; font.letterSpacing: 0.7 }
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: 310
                    Layout.preferredHeight: 28
                    color: "#0b0e14"
                    border.color: "#2a3440"
                    radius: 4
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 9
                        Text { text: "⌕"; color: "#35d9e7"; font.pixelSize: 16 }
                        Text { Layout.fillWidth: true; text: "Search files, symbols, commands…"; color: "#778392"; font.pixelSize: 10 }
                        Text { text: "Ctrl P"; color: "#94a0ad"; font.family: "monospace"; font.pixelSize: 8 }
                    }
                    MouseArea { anchors.fill: parent; onClicked: { palette.visible = true; commandInput.forceActiveFocus() } }
                }
                Rectangle {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 27
                    color: "#111923"
                    border.color: win.modalMode ? "#fb4c9a" : "#2b3943"
                    radius: 4
                    Text { anchors.centerIn: parent; text: win.modalMode ? "◉  HELIX MODE" : "○  INSERT MODE"; color: win.modalMode ? "#fb70ad" : "#9aa7b6"; font.pixelSize: 8; font.family: "monospace"; font.letterSpacing: 0.5 }
                    MouseArea { anchors.fill: parent; onClicked: win.toggleModalMode() }
                }
                Text { text: "●"; color: "#49e27e"; font.pixelSize: 9 }
                Text { text: "LOCAL"; color: "#97a3b0"; font.pixelSize: 8; font.family: "monospace"; font.letterSpacing: 0.8 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 50
                Layout.fillHeight: true
                color: theme.rail
                border.color: "#202833"
                Column {
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 5
                    Repeater {
                        model: ["▤", "⌕", "⑂", "▧"]
                        delegate: Rectangle {
                            required property string modelData
                            required property int index
                            width: 38; height: 38
                            color: index === 0 ? "#151d27" : railMouse.containsMouse ? "#121922" : "transparent"
                            border.color: index === 0 ? "#263743" : "transparent"
                            radius: 4
                            Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? "#45d7e4" : "#8994a2"; font.pixelSize: 16 }
                            Rectangle { width: 2; height: 24; anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; visible: index === 0; color: "#3ad9e6" }
                            MouseArea { id: railMouse; anchors.fill: parent; hoverEnabled: true; onClicked: { if (index === 0) win.explorerVisible = !win.explorerVisible; else if (index === 1) { palette.visible = true; commandInput.forceActiveFocus() } else if (index === 2) { win.activeBottomTab = "SOURCE CONTROL"; win.bottomVisible = true } else { win.activeBottomTab = "EXTENSIONS"; win.bottomVisible = true } } }
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 244
                Layout.fillHeight: true
                visible: win.explorerVisible
                color: theme.sidebar
                border.color: "#202833"
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 43
                        Layout.leftMargin: 15
                        Layout.rightMargin: 11
                        Text { text: "EXPLORER"; color: "#aab4c1"; font.pixelSize: 9; font.family: "monospace"; font.letterSpacing: 1 }
                        Item { Layout.fillWidth: true }
                        Text { text: "···"; color: "#8a96a4"; font.pixelSize: 16 }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#202833" }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Layout.leftMargin: 13
                        Text { text: "⌄"; color: "#798696"; font.pixelSize: 10 }
                        Text { text: "DWCODE"; color: "#c7d0db"; font.pixelSize: 9; font.bold: true; font.family: "monospace"; font.letterSpacing: 0.7 }
                        Item { Layout.fillWidth: true }
                        Text { text: "＋"; color: "#8995a4"; font.pixelSize: 14 }
                        Text { text: "↻"; color: "#8995a4"; font.pixelSize: 13 }
                        Text { text: "▣"; color: "#8995a4"; font.pixelSize: 11; rightPadding: 10 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Repeater {
                            model: win.files
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                height: 29
                                color: win.selectedFile === (modelData.kind === "folder" ? "src/main.ts" : modelData.name) ? "#14202a" : fileHover.containsMouse ? "#151a23" : "transparent"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14 + modelData.depth * 15
                                    anchors.rightMargin: 10
                                    spacing: 8
                                    Text { text: modelData.kind === "folder" ? "⌄" : ""; color: "#8794a4"; font.pixelSize: 11; Layout.preferredWidth: 8 }
                                    Text { text: modelData.kind === "folder" ? "▰" : (modelData.language === "css" ? "◈" : modelData.language === "markdown" ? "M" : "TS"); color: modelData.kind === "folder" ? "#44d6e4" : modelData.language === "css" ? "#ad83ff" : modelData.language === "markdown" ? "#87aaff" : "#4fa8e8"; font.pixelSize: 9; font.family: "monospace"; font.bold: true; Layout.preferredWidth: 15 }
                                    Text { Layout.fillWidth: true; text: modelData.name; color: win.selectedFile === modelData.name ? "#e3e8ef" : "#aab4c1"; font.pixelSize: 10; elide: Text.ElideRight }
                                }
                                MouseArea {
                                    id: fileHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    enabled: modelData.kind === "file"
                                    onClicked: win.openFile(modelData.name)
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#202833" }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        Layout.leftMargin: 13
                        Text { text: "⌄"; color: "#798696"; font.pixelSize: 10 }
                        Text { text: "OUTLINE"; color: "#aab4c1"; font.pixelSize: 9; font.family: "monospace"; font.letterSpacing: 0.7 }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 39
                    spacing: 0
                    Rectangle {
                        Layout.preferredWidth: 174
                        Layout.fillHeight: true
                        color: "#0b0e14"
                        border.color: "#202833"
                        Rectangle { width: 2; height: 2; anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; color: "#43d9e6" }
                        Text { anchors.left: parent.left; anchors.leftMargin: 25; anchors.verticalCenter: parent.verticalCenter; text: win.selectedFile; color: "#d1d8e2"; font.pixelSize: 10; font.family: "monospace" }
                        Text { anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "×"; color: "#788493"; font.pixelSize: 13 }
                        Rectangle { height: 1; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; color: "#35d9e7" }
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        Layout.preferredWidth: 117
                        Layout.preferredHeight: 25
                        color: "#141b24"
                        border.color: "#293743"
                        radius: 4
                        Row {
                            anchors.centerIn: parent
                            spacing: 7
                            Text { text: "▶"; color: "#41dd91"; font.pixelSize: 8 }
                            Text { text: "RUN PREVIEW"; color: "#b5c0cc"; font.pixelSize: 8; font.family: "monospace"; font.letterSpacing: 0.3 }
                        }
                    }
                    Item { width: 12 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 1
                    WebEngineView {
                        id: editor
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        url: webEditorUrl
                        backgroundColor: theme.canvas
                        onLoadingChanged: function(loadRequest) { if (loadRequest.status === WebEngineView.LoadSucceededStatus) runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode(" + (win.modalMode ? "true" : "false") + ")") }
                    }
                    Rectangle { visible: win.splitVisible; Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#27313d" }
                    WebEngineView {
                        id: splitEditor
                        visible: win.splitVisible
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        url: webEditorUrl
                        backgroundColor: theme.canvas
                        onLoadingChanged: function(loadRequest) { if (loadRequest.status === WebEngineView.LoadSucceededStatus) runJavaScript("window.dwcodeOpenSample && window.dwcodeOpenSample(" + JSON.stringify(win.selectedFile) + ")") }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    color: theme.shell
                    border.color: "#242d38"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 16
                        Text { text: "◦  " + (win.modalMode ? "NORMAL" : "INSERT"); color: win.modalMode ? "#fb69a7" : "#39d7e5"; font.family: "monospace"; font.pixelSize: 8; font.letterSpacing: 0.3 }
                        Text { text: "Ln 7, Col 3"; color: "#8b97a5"; font.family: "monospace"; font.pixelSize: 8 }
                        Text { text: "UTF-8"; color: "#8b97a5"; font.family: "monospace"; font.pixelSize: 8 }
                        Item { Layout.fillWidth: true }
                        Text { text: "TYPESCRIPT"; color: "#9eaab7"; font.family: "monospace"; font.pixelSize: 8 }
                        Text { text: "SPACES: 2"; color: "#8b97a5"; font.family: "monospace"; font.pixelSize: 8 }
                        Text { text: "◉ THEME: DEEP CURRENT"; color: "#8b97a5"; font.family: "monospace"; font.pixelSize: 8 }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: win.bottomVisible ? 190 : 0
                    visible: win.bottomVisible
                    color: theme.bottomPanel
                    border.color: "#202a34"
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            Layout.leftMargin: 15
                            Layout.rightMargin: 12
                            spacing: 20
                            Repeater {
                                model: ["OUTPUT", "TERMINAL", "PROBLEMS", "DEBUG CONSOLE"]
                                delegate: Text {
                                    required property string modelData
                                    text: modelData
                                    color: win.activeBottomTab === modelData ? "#d5dde7" : "#768392"
                                    font.pixelSize: 8
                                    font.family: "monospace"
                                    font.letterSpacing: 0.5
                                    Layout.alignment: Qt.AlignVCenter
                                    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 2; color: "#35d9e7"; visible: win.activeBottomTab === modelData }
                                    MouseArea { anchors.fill: parent; onClicked: win.activeBottomTab = modelData }
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Text { text: "⌃"; color: "#8793a0"; font.pixelSize: 12 }
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#242d38" }
                        Column {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.leftMargin: 16
                            Layout.topMargin: 11
                            spacing: 6
                            Text { text: "> dwcode --workspace ./cybercore"; color: "#8c99a8"; font.family: "monospace"; font.pixelSize: 9; visible: win.activeBottomTab === "TERMINAL" }
                            Text { text: "DWCode prototype ready. Monaco is running locally inside the Qt desktop shell."; color: "#acb8c5"; font.family: "monospace"; font.pixelSize: 9; visible: win.activeBottomTab === "OUTPUT" }
                            Text { text: "No problems detected in this preview."; color: "#71808f"; font.family: "monospace"; font.pixelSize: 9; visible: win.activeBottomTab === "PROBLEMS" }
                            Text { text: "Debug adapter not connected."; color: "#71808f"; font.family: "monospace"; font.pixelSize: 9; visible: win.activeBottomTab === "DEBUG CONSOLE" }
                            Text { text: "0 errors   0 warnings"; color: "#788693"; font.family: "monospace"; font.pixelSize: 8; visible: win.activeBottomTab === "PROBLEMS" }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: theme.shell
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 13
                Text { text: "◆"; color: "#35d9e7"; font.pixelSize: 8 }
                Text { text: "dwcode"; color: "#b2becb"; font.family: "monospace"; font.pixelSize: 8 }
                Text { text: "main"; color: "#8d99a6"; font.family: "monospace"; font.pixelSize: 8 }
                Text { text: "⑂ 0"; color: "#8d99a6"; font.family: "monospace"; font.pixelSize: 8 }
                Item { Layout.fillWidth: true }
                Text { text: "DWCODE PREVIEW 0.1"; color: "#86929f"; font.family: "monospace"; font.pixelSize: 8; font.letterSpacing: 0.3 }
                Text { text: "LOCAL WORKSPACE"; color: "#38d9e6"; font.family: "monospace"; font.pixelSize: 8 }
            }
        }
    }

    Rectangle {
        id: palette
        anchors.fill: parent
        z: 20
        visible: false
        color: "#9905080d"
        onVisibleChanged: if (visible) { commandInput.text = ""; commandInput.forceActiveFocus() }
        MouseArea { anchors.fill: parent; onClicked: palette.visible = false }
        Rectangle {
            width: 620
            height: 354
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 82
            color: "#11161f"
            border.color: "#34414d"
            radius: 7
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 9
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "#0b0e14"
                    border.color: "#35cbd9"
                    radius: 4
                    TextField {
                        id: commandInput
                        anchors.fill: parent
                        leftPadding: 13
                        placeholderText: "Type a command or filename…"
                        color: "#e5ebf2"
                        placeholderTextColor: "#74808e"
                        font.pixelSize: 12
                        background: Item {}
                        onTextChanged: win.paletteFilter = text.toLowerCase()
                        Keys.onReturnPressed: {
                            for (let i = 0; i < win.commands.length; ++i) {
                                if (win.commands[i].title.toLowerCase().indexOf(win.paletteFilter) >= 0) {
                                    win.activateCommand(win.commands[i].action)
                                    break
                                }
                            }
                        }
                    }
                }
                Text { text: "COMMANDS"; color: "#7f8b99"; font.pixelSize: 8; font.family: "monospace"; font.letterSpacing: 1; Layout.leftMargin: 4 }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: win.commands
                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view.width
                        height: win.paletteFilter.length === 0 || modelData.title.toLowerCase().indexOf(win.paletteFilter) >= 0 ? 38 : 0
                        visible: height > 0
                        color: commandHover.containsMouse ? "#1a2631" : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Text { Layout.fillWidth: true; text: modelData.title; color: "#cbd4df"; font.pixelSize: 10 }
                            Text { text: modelData.shortcut; color: "#7d8997"; font.family: "monospace"; font.pixelSize: 8 }
                        }
                        MouseArea { id: commandHover; anchors.fill: parent; hoverEnabled: true; onClicked: win.activateCommand(modelData.action) }
                    }
                }
                Text { text: "DWCODE  /  KEYBOARD FIRST"; color: "#56616f"; font.pixelSize: 8; font.family: "monospace"; Layout.leftMargin: 4 }
            }
        }
    }
}
