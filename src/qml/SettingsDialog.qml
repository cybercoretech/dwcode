import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
 id: root
 objectName: "settingsDialog"
 title: "Settings"
 modal: true
 anchors.centerIn: parent
 width: Math.min(parent ? parent.width - 56 : 940, 940)
 height: Math.min(parent ? parent.height - 56 : 720, 720)
 property string query: ""
 property var preferences: workspaceBackend.preferences
 function value(key, fallback) { const value=preferences[key]; return value === undefined ? fallback : value }
 function setValue(key, value) { workspaceBackend.setPreference(key, value) }
 function indexOf(values, value) { for(let i=0;i<values.length;i++) if(values[i]===value) return i; return 0 }
 function matches(words) { return !query || words.toLowerCase().includes(query.toLowerCase()) }

 background: Rectangle { color: theme.elevated; border.color: theme.border; radius: 8 }
 header: Rectangle {
  height: 58; color: theme.shell; border.color: theme.border
  RowLayout { anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 14; spacing: 12
   ColumnLayout { spacing: 1
    Label { text: "SETTINGS"; color: theme.foreground; font.bold: true; font.pixelSize: 15 }
    Label { text: "Changes apply immediately and are saved locally."; color: theme.mutedText; font.pixelSize: 10 }
   }
   Item { Layout.fillWidth: true }
   ToolButton { text: "×"; Accessible.name: "Close settings"; onClicked: root.close() }
  }
 }
 footer: Rectangle {
  height: 48; color: theme.shell; border.color: theme.border
  RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
   Label { text: "Settings are global to DWCode on this device."; color: theme.mutedText; font.pixelSize: 10; Layout.fillWidth: true }
   Button { text: "Reset all"; onClicked: workspaceBackend.resetPreferences() }
   Button { text: "Done"; highlighted: true; onClicked: root.close() }
  }
 }
 contentItem: ColumnLayout {
  spacing: 0
  Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 56; color: theme.background
   TextField { id: search; anchors.fill: parent; anchors.margins: 12; placeholderText: "Search settings…"; text: root.query; onTextChanged: root.query=text; Accessible.name: "Search settings"
    background: Rectangle { color: theme.canvas; border.color: search.activeFocus ? theme.cyan : theme.border; radius: 4 }
   }
  }
  ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
   ColumnLayout { width: parent.width; spacing: 0
    Rectangle { visible: root.matches("appearance theme scale density sidebar status bar"); Layout.fillWidth: true; implicitHeight: appearanceColumn.implicitHeight + 28; color: "transparent"
     ColumnLayout { id: appearanceColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "APPEARANCE"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Color theme"; Layout.preferredWidth: 235; color: theme.foreground }
       ComboBox { Layout.fillWidth: true; model: workspaceBackend.themes; textRole: "name"; valueRole: "id"
        currentIndex: { for(let i=0;i<workspaceBackend.themes.length;i++) if(workspaceBackend.themes[i].id===workspaceBackend.theme.id)return i; return 0 }
        onActivated: workspaceBackend.setTheme(currentValue)
       }
      }
      RowLayout { Layout.fillWidth: true; Label { text: "Interface scale"; Layout.preferredWidth: 235; color: theme.foreground }
       Slider { id: uiScale; Layout.fillWidth: true; from: 80; to: 150; stepSize: 5; value: root.value("appearance/uiScale",100); onMoved: root.setValue("appearance/uiScale",Math.round(value)) }
       Label { text: Math.round(uiScale.value)+"%"; color: theme.secondaryText; Layout.preferredWidth: 42 }
      }
      RowLayout { Layout.fillWidth: true; Label { text: "Layout density"; Layout.preferredWidth: 235; color: theme.foreground }
       ComboBox { Layout.fillWidth: true; model: ["comfortable","compact"]; currentIndex: root.indexOf(model,root.value("appearance/density","comfortable")); onActivated: root.setValue("appearance/density",currentText) }
      }
      RowLayout { Layout.fillWidth: true; Label { text: "Explorer side"; Layout.preferredWidth: 235; color: theme.foreground }
       ComboBox { Layout.fillWidth: true; model: ["left","right"]; currentIndex: root.indexOf(model,root.value("appearance/sidebarPosition","left")); onActivated: root.setValue("appearance/sidebarPosition",currentText) }
      }
      RowLayout { Layout.fillWidth: true; Label { text: "Show status bar"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("appearance/statusBar",true); onToggled: root.setValue("appearance/statusBar",checked) } }
     }
    }
    Rectangle { visible: root.matches("editor font ligatures line height wrap minimap lines rulers whitespace guides highlight scrolling"); Layout.fillWidth: true; implicitHeight: editorColumn.implicitHeight + 28; color: theme.background
     ColumnLayout { id: editorColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "EDITOR"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Font family"; Layout.preferredWidth: 235; color: theme.foreground } TextField { Layout.fillWidth: true; text: root.value("editor/fontFamily",""); onEditingFinished: root.setValue("editor/fontFamily",text) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Font size"; Layout.preferredWidth: 235; color: theme.foreground } SpinBox { from: 9; to: 32; value: root.value("editor/fontSize",14); onValueModified: root.setValue("editor/fontSize",value) } Item { Layout.fillWidth: true } Label { text: "Line height"; color: theme.foreground } SpinBox { from: 12; to: 48; value: root.value("editor/lineHeight",23); onValueModified: root.setValue("editor/lineHeight",value) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Word wrap"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["off","on","bounded"]; currentIndex: root.indexOf(model,root.value("editor/wordWrap","off")); onActivated: root.setValue("editor/wordWrap",currentText) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Line numbers"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["on","relative","off"]; currentIndex: root.indexOf(model,root.value("editor/lineNumbers","on")); onActivated: root.setValue("editor/lineNumbers",currentText) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Rulers (columns, comma-separated)"; Layout.preferredWidth: 235; color: theme.foreground } TextField { Layout.fillWidth: true; placeholderText: "e.g. 80, 100"; text: root.value("editor/rulers",""); onEditingFinished: root.setValue("editor/rulers",text) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Render whitespace"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["none","boundary","all"]; currentIndex: root.indexOf(model,root.value("editor/renderWhitespace","none")); onActivated: root.setValue("editor/renderWhitespace",currentText) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Font ligatures"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editor/fontLigatures",true); onToggled: root.setValue("editor/fontLigatures",checked) } Item { Layout.fillWidth: true } Label { text: "Minimap"; color: theme.foreground } Switch { checked: root.value("editor/minimap",true); onToggled: root.setValue("editor/minimap",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Bracket guides"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editor/bracketGuides",true); onToggled: root.setValue("editor/bracketGuides",checked) } Item { Layout.fillWidth: true } Label { text: "Current-line highlight"; color: theme.foreground } Switch { checked: root.value("editor/currentLineHighlight",true); onToggled: root.setValue("editor/currentLineHighlight",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Smooth scrolling"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editor/smoothScrolling",true); onToggled: root.setValue("editor/smoothScrolling",checked) } }
     }
    }
    Rectangle { visible: root.matches("editing indent tabs spaces brackets save whitespace newline endings"); Layout.fillWidth: true; implicitHeight: editingColumn.implicitHeight + 28; color: "transparent"
     ColumnLayout { id: editingColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "EDITING"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Tab width"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: [2,4,8]; currentIndex: root.indexOf(model,root.value("editing/tabSize",2)); onActivated: root.setValue("editing/tabSize",currentValue) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Indent using spaces"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editing/insertSpaces",true); onToggled: root.setValue("editing/insertSpaces",checked) } Item { Layout.fillWidth: true } Label { text: "Auto-indent"; color: theme.foreground } Switch { checked: root.value("editing/autoIndent",true); onToggled: root.setValue("editing/autoIndent",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Auto-close brackets and quotes"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editing/autoClosingBrackets",true); onToggled: root.setValue("editing/autoClosingBrackets",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Trim trailing whitespace on save"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("editing/trimTrailingWhitespace",false); onToggled: root.setValue("editing/trimTrailingWhitespace",checked) } Item { Layout.fillWidth: true } Label { text: "Insert final newline"; color: theme.foreground } Switch { checked: root.value("editing/insertFinalNewline",false); onToggled: root.setValue("editing/insertFinalNewline",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Line endings when saving"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["auto","lf","crlf"]; currentIndex: root.indexOf(model,root.value("editing/lineEnding","auto")); onActivated: root.setValue("editing/lineEnding",currentText) } }
     }
    }
    Rectangle { visible: root.matches("files workspace auto save delay exclude explorer restore reopen session"); Layout.fillWidth: true; implicitHeight: filesColumn.implicitHeight + 28; color: theme.background
     ColumnLayout { id: filesColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "FILES & WORKSPACE"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Auto save"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["off","after delay"]; currentIndex: root.value("files/autoSave","off")==="after delay" ? 1 : 0; onActivated: root.setValue("files/autoSave",currentText) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Auto-save delay"; Layout.preferredWidth: 235; color: theme.foreground } SpinBox { from: 250; to: 10000; stepSize: 250; editable: true; value: root.value("files/autoSaveDelay",1000); onValueModified: root.setValue("files/autoSaveDelay",value) } Label { text: "milliseconds"; color: theme.mutedText; Layout.fillWidth: true } }
      RowLayout { Layout.fillWidth: true; Label { text: "Reopen files from the last session"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("files/restoreSession",true); onToggled: root.setValue("files/restoreSession",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Explorer exclude patterns"; Layout.preferredWidth: 235; color: theme.foreground } TextField { Layout.fillWidth: true; placeholderText: "e.g. *.log; coverage; generated/*"; text: root.value("files/excludes",""); onEditingFinished: root.setValue("files/excludes",text) } }
      Label { text: "Separate patterns with semicolons. Core build and dependency folders remain excluded for responsiveness."; color: theme.mutedText; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
     }
    }
    Rectangle { visible: root.matches("terminal shell directory font scrollback selection clear environment dangerous warning"); Layout.fillWidth: true; implicitHeight: terminalColumn.implicitHeight + 28; color: "transparent"
     ColumnLayout { id: terminalColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "TERMINAL"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Shell"; Layout.preferredWidth: 235; color: theme.foreground } TextField { Layout.fillWidth: true; text: root.value("terminal/shell","/bin/bash"); onEditingFinished: root.setValue("terminal/shell",text) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Working directory"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["workspace","home"]; currentIndex: root.indexOf(model,root.value("terminal/workingDirectory","workspace")); onActivated: root.setValue("terminal/workingDirectory",currentText) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Terminal font"; Layout.preferredWidth: 235; color: theme.foreground } TextField { Layout.fillWidth: true; text: root.value("terminal/fontFamily","monospace"); onEditingFinished: root.setValue("terminal/fontFamily",text) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Font size"; Layout.preferredWidth: 235; color: theme.foreground } SpinBox { from: 8; to: 28; value: root.value("terminal/fontSize",11); onValueModified: root.setValue("terminal/fontSize",value) } Item { Layout.fillWidth: true } Label { text: "Scrollback"; color: theme.foreground } ComboBox { model: [50000,100000,200000,500000]; currentIndex: root.indexOf(model,root.value("terminal/scrollback",200000)); onActivated: root.setValue("terminal/scrollback",currentValue) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Copy text by selecting"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("terminal/copyOnSelect",true); onToggled: root.setValue("terminal/copyOnSelect",checked) } Item { Layout.fillWidth: true } Label { text: "Clear before each command"; color: theme.foreground } Switch { checked: root.value("terminal/clearOnRun",false); onToggled: root.setValue("terminal/clearOnRun",checked) } }
      RowLayout { Layout.fillWidth: true; Layout.alignment: Qt.AlignTop
       Label { text: "Environment additions"; Layout.preferredWidth: 235; color: theme.foreground; Layout.alignment: Qt.AlignTop }
       TextArea { Layout.fillWidth: true; Layout.preferredHeight: 72; placeholderText: "NAME=value (one per line)"; wrapMode: TextEdit.NoWrap; text: root.value("terminal/environment","")
        onTextChanged: if (activeFocus) root.setValue("terminal/environment",text)
       }
      }
      RowLayout { Layout.fillWidth: true; Label { text: "Warn before risky commands"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("terminal/warnRisky",true); onToggled: root.setValue("terminal/warnRisky",checked) } }
     }
    }
    Rectangle { visible: root.matches("keyboard modal helix cursor"); Layout.fillWidth: true; implicitHeight: keyboardColumn.implicitHeight + 28; color: theme.background
     ColumnLayout { id: keyboardColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; spacing: 10
      Label { text: "KEYBOARD & MODAL EDITING"; color: theme.cyan; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
      RowLayout { Layout.fillWidth: true; Label { text: "Enable Helix mode by default"; Layout.preferredWidth: 235; color: theme.foreground } Switch { checked: root.value("keyboard/modalDefault",false); onToggled: root.setValue("keyboard/modalDefault",checked) } }
      RowLayout { Layout.fillWidth: true; Label { text: "Normal-mode cursor"; Layout.preferredWidth: 235; color: theme.foreground } ComboBox { Layout.fillWidth: true; model: ["block","line"]; currentIndex: root.indexOf(model,root.value("keyboard/modalCursor","block")); onActivated: root.setValue("keyboard/modalCursor",currentText) } }
      Label { text: "Built-in commands keep their documented shortcuts. Custom keymaps will arrive with the reviewed keymap-pack system."; color: theme.mutedText; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
     }
    }
    Label { visible: root.query && !root.matches("appearance theme scale density sidebar status bar editor font ligatures line height wrap minimap lines rulers whitespace guides highlight scrolling editing indent tabs spaces brackets save newline endings files workspace auto delay exclude explorer restore reopen session terminal shell directory scrollback selection clear environment dangerous warning keyboard modal helix cursor"); text: "No settings match ‘"+root.query+"’."; color: theme.mutedText; Layout.margins: 24 }
   }
  }
 }
}
