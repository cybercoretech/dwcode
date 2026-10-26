pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtWebEngine
import QtWebChannel

ApplicationWindow {
 id: win
 width: 1440; height: 900; minimumWidth: 1050; minimumHeight: 640
 visible: true; title: "DWCode — " + (workspaceBackend.root || "Open a workspace")
 color: theme.background
 property bool explorerVisible: true
 property bool bottomVisible: true
 property bool splitVisible: false
 property bool modalMode: false
 property bool paletteVisible: false
 property string activeBottomTab: "OUTPUT"
 property string pendingAction: ""
 property bool closingApproved: false
 property real uiScale: Number(workspaceBackend.preferences["appearance/uiScale"] || 100) / 100
 property bool compactUi: workspaceBackend.preferences["appearance/density"] === "compact"
 property bool sidebarRight: workspaceBackend.preferences["appearance/sidebarPosition"] === "right"
 property int tabHeight: Math.round((compactUi ? 32 : 38) * uiScale)
 property var commands: [
  { title: "Open Folder", shortcut: "Ctrl+O", action: "folder" },
  { title: "New File", shortcut: "Ctrl+N", action: "newfile" },
  { title: "Save File", shortcut: "Ctrl+S", action: "save" },
  { title: "Save File As", shortcut: "Ctrl+Shift+S", action: "saveas" },
  { title: "Toggle File Explorer", shortcut: "Ctrl+B", action: "explorer" },
  { title: "Toggle Terminal", shortcut: "Ctrl+J", action: "terminal" },
  { title: "Show Problems", shortcut: "Ctrl+Shift+M", action: "problems" },
  { title: "Split Editor", shortcut: "Ctrl+\\", action: "split" },
  { title: "Connect Language Server", shortcut: "", action: "lsp" },
  { title: "Disconnect Language Server", shortcut: "", action: "lspstop" },
  { title: "Toggle Helix Mode", shortcut: "Ctrl+Alt+M", action: "modal" }
 ]
 Theme { id: theme }
 palette.window: theme.background; palette.windowText: theme.foreground; palette.base: theme.canvas
 palette.text: theme.foreground; palette.button: theme.elevated; palette.buttonText: theme.foreground
 palette.highlight: theme.cyan; palette.highlightedText: theme.background
 palette.placeholderText: theme.mutedText
 function save(){editor.runJavaScript("window.dwcodeSave && window.dwcodeSave()")}
 function chooseFolder(){if(workspaceBackend.hasUnsaved()){pendingAction="folder";unsaved.open()}else folderDialog.open()}
 function activate(action){
  paletteVisible=false
  if(action==="folder")chooseFolder()
  else if(action==="newfile")newFileDialog.open()
  else if(action==="save")save()
  else if(action==="saveas")saveDialog.open()
  else if(action==="explorer")explorerVisible=!explorerVisible
  else if(action==="terminal"){bottomVisible=!bottomVisible;activeBottomTab="TERMINAL"}
  else if(action==="output"){bottomVisible=true;activeBottomTab="OUTPUT"}
  else if(action==="problems"){bottomVisible=true;activeBottomTab="PROBLEMS"}
  else if(action==="split")splitVisible=!splitVisible
  else if(action==="lsp")workspaceBackend.startLanguageServer()
  else if(action==="lspstop")workspaceBackend.stopLanguageServer()
  else if(action==="refresh")workspaceBackend.refreshFiles()
  else if(action==="palette")paletteVisible=true
  else if(action==="settings")settingsDialog.open()
  else if(action==="undo")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','undo',{})")
  else if(action==="redo")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','redo',{})")
  else if(action==="selectall")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','editor.action.selectAll',{})")
  else if(action==="modal"){modalMode=!modalMode;editor.runJavaScript("window.dwcodeSetMode("+modalMode+")");splitEditor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode("+modalMode+")")}
 }
 function runTerminal(command){
  const risky=/(^|\s)(sudo|rm\s+-[^\n]*r|git\s+reset\s+--hard|git\s+clean\s+-[fd]|mkfs|dd\s+if=|chmod\s+-R)(\s|$)/.test(command)
  if(workspaceBackend.preferences["terminal/warnRisky"]&&risky){pendingCommand=command;riskyCommand.open()}else workspaceBackend.runCommand(command)
 }
 FolderDialog {id:folderDialog;objectName:"folderDialog";title:"Open DWCode workspace";onAccepted:workspaceBackend.openWorkspace(selectedFolder.toString())}
 FileDialog {id:saveDialog;title:"Save file as (inside workspace)";fileMode:FileDialog.SaveFile;onAccepted:workspaceBackend.saveAs(selectedFile.toString())}
 Dialog { id: newFileDialog; objectName: "newFileDialog"; title: "New File"; anchors.centerIn: parent; modal: true; width: 480
  onOpened: { newFilePath.text=""; newFilePath.forceActiveFocus() }
  contentItem: ColumnLayout { spacing: 10
   Label { text: "Create an unsaved file inside the current workspace."; color: theme.secondaryText; wrapMode: Text.WordWrap; Layout.fillWidth: true }
   TextField { id: newFilePath; objectName: "newFilePath"; Layout.fillWidth: true; placeholderText: "e.g. src/new-feature.ts"; onAccepted: if(workspaceBackend.createFile(text))newFileDialog.close() }
  }
  footer: DialogButtonBox { Button { text: "Cancel"; onClicked: newFileDialog.close() } Button { text: "Create"; highlighted: true; onClicked: if(workspaceBackend.createFile(newFilePath.text))newFileDialog.close() } }
 }
 SettingsDialog { id: settingsDialog }
 property string pendingCommand: ""
 Dialog { id: riskyCommand; title: "Run potentially risky command?"; anchors.centerIn: parent; modal: true; standardButtons: Dialog.Cancel
  Label { width: 420; wrapMode: Text.WordWrap; text: "DWCode detected a command that can modify or remove data:\n\n"+win.pendingCommand+"\n\nReview it before continuing."; color: theme.foreground }
  footer: DialogButtonBox { Button { text: "Cancel"; onClicked: riskyCommand.close() } Button { text: "Run command"; highlighted: true; onClicked: { riskyCommand.close(); workspaceBackend.runCommand(win.pendingCommand) } } }
 }
 Dialog {id:unsaved;objectName:"unsavedDialog";title:"Unsaved files";anchors.centerIn:parent;modal:true;width:520
  Label {width:470;wrapMode:Text.WordWrap;color:theme.foreground
   text:win.pendingAction==="close"
    ? "You have unsaved changes. Keep them to reopen this session next time DWCode starts, or discard them."
    : "You have unsaved changes. Discard them to open a different folder."}
  footer: DialogButtonBox {
   Button {text:"Discard";onClicked:{unsaved.close();workspaceBackend.discardChanges();if(pendingAction==="close"){closingApproved=true;win.close()}else folderDialog.open()}}
   Button {visible:win.pendingAction==="close";text:"Keep Changes";highlighted:true
    onClicked:{unsaved.close();closingApproved=true;win.close()}}
   Button {text:"Cancel";onClicked:unsaved.close()}
  }
 }
 property var externalDocument: ({})
 Dialog { id: externalChangeDialog; objectName: "externalChangeDialog"; title: "File changed outside DWCode"; anchors.centerIn: parent; modal: true; width: 510
  Label { width: 450; wrapMode: Text.WordWrap; color: theme.foreground; text: externalDocument.deleted ? externalDocument.name+" was deleted outside DWCode. Keep this editor copy, then Save to recreate it or Save As elsewhere." : externalDocument.name+" changed outside DWCode."+(externalDocument.dirty ? " Your unsaved edits are preserved." : " Reload it to use the disk version.") }
  footer: DialogButtonBox { Button { text: "Keep Editor Copy"; onClicked: externalChangeDialog.close() } Button { visible: !externalDocument.deleted; text: "Reload from Disk"; highlighted: true; onClicked: { if(workspaceBackend.reloadDocument(externalDocument.path))externalChangeDialog.close() } } }
 }
 Connections { target: workspaceBackend; function onDocumentExternalChange(document) { win.externalDocument=document; externalChangeDialog.open() } }
 onClosing:function(event){if(!closingApproved&&workspaceBackend.hasUnsaved()){event.accepted=false;pendingAction="close";unsaved.open()}}
 WebChannel {id:channel;Component.onCompleted:registerObject("native",workspaceBackend)}
 WebChannel {id:splitChannel;Component.onCompleted:registerObject("native",workspaceBackend)}
 Component.onCompleted:{modalMode=workspaceBackend.preferences["keyboard/modalDefault"];if(!workspaceBackend.restoreSession()&&workspaceBackend.files.length>0){for(let i=0;i<workspaceBackend.files.length;i++){let f=workspaceBackend.files[i];if(f.kind==="file"){workspaceBackend.openFile(f.path);break}}}}
 onModalModeChanged:{editor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode("+modalMode+")");splitEditor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode("+modalMode+")")}
 Shortcut {sequence:"Ctrl+O";onActivated:chooseFolder()}
 Shortcut {sequence:"Ctrl+N";onActivated:newFileDialog.open()}
 Shortcut {sequence:"Ctrl+S";onActivated:save()}
 Shortcut {sequence:"Ctrl+Shift+S";onActivated:saveDialog.open()}
 Shortcut {sequence:"Ctrl+P";onActivated:win.paletteVisible=true}
 Shortcut {sequence:"Ctrl+Shift+P";onActivated:win.paletteVisible=true}
 Shortcut {sequence:"Ctrl+B";onActivated:win.explorerVisible=!win.explorerVisible}
 Shortcut {sequence:"Ctrl+J";onActivated:activate("terminal")}
 Shortcut {sequence:"Ctrl+\\";onActivated:win.splitVisible=!win.splitVisible}
 Shortcut {sequence:"Ctrl+Alt+M";onActivated:activate("modal")}
 Shortcut {sequence:"Ctrl+,";onActivated:activate("settings")}
 Shortcut {sequence:"Escape";enabled:win.paletteVisible;onActivated:win.paletteVisible=false}
 ColumnLayout {
  anchors.fill:parent;spacing:0
  Rectangle {
   Layout.fillWidth:true;Layout.preferredHeight:Math.round((compactUi?54:64)*uiScale);color:theme.shell;border.color:theme.border
   ColumnLayout {anchors.fill:parent;spacing:0
    RowLayout {Layout.fillWidth:true;Layout.preferredHeight:31;Layout.leftMargin:10;Layout.rightMargin:10;spacing:10
     Image {source:"qrc:/qml/dwcode-icon.png";Layout.preferredHeight:18;Layout.preferredWidth:18;fillMode:Image.PreserveAspectFit;smooth:true;mipmap:true;Accessible.name:"DWCode"}
     MenuBar {id:applicationMenuBar;objectName:"applicationMenuBar";Layout.fillWidth:true;Layout.preferredHeight:31
      background:Rectangle{color:"transparent"}
      Menu {title:"File"
       MenuItem{text:"Open Folder…";onTriggered:chooseFolder()}
       MenuItem{text:"New File…";onTriggered:newFileDialog.open()}
       MenuSeparator{}
       MenuItem{text:"Save";enabled:workspaceBackend.activePath!=="";onTriggered:save()}
       MenuItem{text:"Save As…";enabled:workspaceBackend.activePath!=="";onTriggered:saveDialog.open()}
      }
      Menu {title:"Edit"
       MenuItem{text:"Undo";onTriggered:activate("undo")}
       MenuItem{text:"Redo";onTriggered:activate("redo")}
       MenuSeparator{}
       MenuItem{text:"Select All";onTriggered:activate("selectall")}
       MenuItem{text:"Command Palette…";onTriggered:activate("palette")}
      }
      Menu {title:"Selection"
       MenuItem{text:"Select All";onTriggered:activate("selectall")}
       MenuItem{text:modalMode?"Disable Helix Mode":"Enable Helix Mode";onTriggered:activate("modal")}
      }
      Menu {title:"View"
       MenuItem{text:explorerVisible?"Hide Explorer":"Show Explorer";onTriggered:activate("explorer")}
       MenuItem{text:bottomVisible?"Hide Terminal":"Show Terminal";onTriggered:activate("terminal")}
       MenuItem{text:"Show Output";onTriggered:activate("output")}
       MenuItem{text:"Show Problems";onTriggered:activate("problems")}
       MenuSeparator{}
       MenuItem{text:splitVisible?"Single Editor":"Split Editor";onTriggered:activate("split")}
      }
      Menu {title:"Run"
       MenuItem{text:"Connect Language Server";enabled:!workspaceBackend.lspReady;onTriggered:activate("lsp")}
       MenuItem{text:"Disconnect Language Server";enabled:workspaceBackend.lspReady;onTriggered:activate("lspstop")}
      }
      Menu {title:"Terminal"
       MenuItem{text:"Toggle Terminal";onTriggered:activate("terminal")}
       MenuItem{text:"Clear Terminal";onTriggered:workspaceBackend.clearTerminal()}
      }
      Menu {title:"Settings"
       MenuItem{text:"Preferences… (Ctrl+,)";onTriggered:activate("settings")}
      }
      Menu {title:"Help"
       MenuItem{text:"Command Palette…";onTriggered:activate("palette")}
      }
     }
     Label{text:"CYBERCORE";color:theme.mutedText;font.pixelSize:9;font.letterSpacing:1}
     ComboBox {id:themePicker;Layout.preferredWidth:200;Layout.preferredHeight:24;model:workspaceBackend.themes;textRole:"name";valueRole:"id"
      currentIndex:{for(let i=0;i<workspaceBackend.themes.length;i++)if(workspaceBackend.themes[i].id===workspaceBackend.theme.id)return i;return 0}
      onActivated:workspaceBackend.setTheme(currentValue)
      popup.height:Math.min(420,themePicker.count*36+16)
     }
     ToolButton{text:workspaceBackend.lspReady?"LSP connected":"Connect LSP";Accessible.name:"Connect language server";onClicked:workspaceBackend.startLanguageServer()}
    }
    Rectangle{Layout.fillWidth:true;Layout.preferredHeight:1;color:theme.border}
    RowLayout {Layout.fillWidth:true;Layout.preferredHeight:32;Layout.leftMargin:10;Layout.rightMargin:10;spacing:8
     Image {source:"qrc:/qml/dwcode-wordmark.png";Layout.preferredHeight:16;Layout.preferredWidth:sourceSize.width*16/Math.max(1,sourceSize.height);fillMode:Image.PreserveAspectFit;smooth:true;mipmap:true;Accessible.name:"DWCode"}
     Label{text:workspaceBackend.root?workspaceBackend.root:"Open a workspace";Layout.fillWidth:true;elide:Text.ElideMiddle;color:theme.secondaryText;font.pixelSize:11}
     ToolButton{text:"↻";Accessible.name:"Refresh explorer";onClicked:activate("refresh")}
     ToolButton{text:workspaceBackend.dirty?"Save ●":"Save";enabled:workspaceBackend.activePath!=="";onClicked:save()}
     ToolButton{text:"⌘";Accessible.name:"Open command palette";onClicked:activate("palette")}
    }
   }
  }
  RowLayout { layoutDirection: win.sidebarRight ? Qt.RightToLeft : Qt.LeftToRight
   Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
   Rectangle {Layout.preferredWidth:235;Layout.fillHeight:true;visible:win.explorerVisible;color:theme.sidebar;border.color:theme.border
    ColumnLayout {anchors.fill:parent;spacing:0
     RowLayout {Layout.fillWidth:true;Layout.margins:12
      Label{text:"EXPLORER";color:theme.secondaryText;font.pixelSize:10;font.letterSpacing:1}
      Item{Layout.fillWidth:true}
      Button{text:"Open Folder";onClicked:chooseFolder()}
      ToolButton{text:"+";Accessible.name:"New file";onClicked:newFileDialog.open()}
      ToolButton{text:"↻";Accessible.name:"Refresh explorer";onClicked:workspaceBackend.refreshFiles()}
     }
     Label{text:workspaceBackend.root.split('/').pop()||"NO WORKSPACE";color:theme.cyan;Layout.leftMargin:14;font.bold:true;font.pixelSize:11}
     ListView {Layout.fillWidth:true;Layout.fillHeight:true;clip:true;model:workspaceBackend.files;ScrollBar.vertical:ScrollBar{}
      delegate:Rectangle {required property var modelData;width:ListView.view.width;height:28
       color:workspaceBackend.activePath.endsWith('/'+modelData.path)?theme.elevated:"transparent"
       Row {anchors.verticalCenter:parent.verticalCenter;x:12+modelData.depth*12;spacing:7
        Text{text:modelData.kind==="folder"?"▾":"◇";color:theme.cyan;font.pixelSize:10}
        Text{text:modelData.name;color:modelData.kind==="folder"?theme.foreground:theme.secondaryText;font.pixelSize:11}
       }
       MouseArea{anchors.fill:parent;enabled:modelData.kind==="file";onClicked:workspaceBackend.openFile(modelData.path)}
      }
     }
    }
   }
   ColumnLayout {Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:win.tabHeight;color:theme.shell;border.color:theme.border
     RowLayout {anchors.fill:parent;spacing:1
      Flickable {Layout.fillWidth:true;Layout.fillHeight:true;contentWidth:tabs.width;clip:true
       Row{id:tabs;height:parent.height
        Repeater{model:workspaceBackend.documents;delegate:Button{id:tabButton;required property var modelData;property bool activeTab:workspaceBackend.activePath===modelData.path;height:win.tabHeight;leftPadding:15;rightPadding:15;text:(modelData.dirty?"● ":"")+modelData.name;flat:true;highlighted:activeTab;onClicked:workspaceBackend.openFile(modelData.path)
          background:Rectangle{color:tabButton.activeTab?theme.canvas:"transparent";border.color:theme.border
           Rectangle{visible:tabButton.activeTab;anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;height:2;color:theme.cyan}
          }
        }}
       }
      }
      ToolButton{text:win.splitVisible?"Single view":"Split editor";onClicked:win.splitVisible=!win.splitVisible}
     }
    }
    RowLayout {Layout.fillWidth:true;Layout.fillHeight:true;spacing:1
     WebEngineView {id:editor;objectName:"editorView";Layout.fillWidth:true;Layout.fillHeight:true;url:webEditorUrl;webChannel:channel;backgroundColor:theme.canvas
      onNavigationRequested:function(request){if(!request.url.toString().startsWith(webEditorUrl.toString()))request.action=WebEngineNavigationRequest.IgnoreRequest}
     }
     WebEngineView {id:splitEditor;Layout.fillWidth:true;Layout.fillHeight:true;visible:win.splitVisible;url:win.splitVisible?webEditorUrl:"about:blank";webChannel:splitChannel;backgroundColor:theme.canvas
      onNavigationRequested:function(request){if(request.url.toString()!=="about:blank"&&!request.url.toString().startsWith(webEditorUrl.toString()))request.action=WebEngineNavigationRequest.IgnoreRequest}
     }
    }
    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:26;color:theme.shell;border.color:theme.border
     RowLayout{anchors.fill:parent;anchors.leftMargin:12;anchors.rightMargin:12
      Label{text:win.modalMode?"NORMAL / h j k l · i to insert":"INSERT";color:win.modalMode?theme.magenta:theme.cyan;font.pixelSize:10}
      Item{Layout.fillWidth:true}Label{text:workspaceBackend.lspStatus;color:theme.mutedText;font.pixelSize:10}
      Label{text:workspaceBackend.theme.name;color:theme.secondaryText;font.pixelSize:10}
     }
    }
    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:win.bottomVisible?220:0;visible:win.bottomVisible;color:theme.bottomPanel;border.color:theme.border
     ColumnLayout{anchors.fill:parent;spacing:0
      RowLayout{Layout.fillWidth:true;Layout.leftMargin:10;Layout.rightMargin:10
       Repeater{model:["OUTPUT","TERMINAL","PROBLEMS"];delegate:ToolButton{required property string modelData;text:modelData+(modelData==="PROBLEMS"?" ("+workspaceBackend.problems.length+")":"");font.pixelSize:10;highlighted:win.activeBottomTab===modelData;onClicked:win.activeBottomTab=modelData}}
       Item{Layout.fillWidth:true}ToolButton{text:"Clear";visible:win.activeBottomTab==="TERMINAL";onClicked:workspaceBackend.clearTerminal()}
       ToolButton{text:"Stop";enabled:workspaceBackend.terminalRunning;visible:win.activeBottomTab==="TERMINAL";onClicked:workspaceBackend.stopCommand()}
      }
      ScrollView{Layout.fillWidth:true;Layout.fillHeight:true;visible:win.activeBottomTab!=="PROBLEMS";clip:true
       TextArea{id:consoleText;readOnly:true;wrapMode:TextEdit.Wrap;color:theme.secondaryText;background:null
        text:win.activeBottomTab==="TERMINAL"?workspaceBackend.terminalOutput:workspaceBackend.message+"\n\n"+workspaceBackend.lspStatus;font.family:win.activeBottomTab==="TERMINAL"?workspaceBackend.preferences["terminal/fontFamily"]:"monospace";font.pixelSize:win.activeBottomTab==="TERMINAL"?workspaceBackend.preferences["terminal/fontSize"]:11;selectByMouse:win.activeBottomTab==="TERMINAL"?workspaceBackend.preferences["terminal/copyOnSelect"]:true
        onTextChanged:if(win.activeBottomTab==="TERMINAL")cursorPosition=length
       }
      }
      ListView{Layout.fillWidth:true;Layout.fillHeight:true;visible:win.activeBottomTab==="PROBLEMS";clip:true;model:workspaceBackend.problems
       delegate:ItemDelegate{required property var modelData;width:ListView.view.width;text:modelData.path.split('/').pop()+":"+(modelData.range.start.line+1)+"  "+modelData.message;font.pixelSize:11;onClicked:workspaceBackend.openFile(modelData.path)}
      }
      RowLayout{Layout.fillWidth:true;Layout.margins:8;visible:win.activeBottomTab==="TERMINAL"
       Label{text:workspaceBackend.terminalRunning?"stdin ›":"$";color:theme.cyan}
       TextField{id:commandLine;Layout.fillWidth:true;placeholderText:workspaceBackend.terminalRunning?"Send input to running command…":"Run a command in the workspace…";font.family:"monospace";font.pixelSize:11
        onAccepted:{if(workspaceBackend.terminalRunning)workspaceBackend.sendInput(text);else win.runTerminal(text);text=""}
       }
       Button{text:workspaceBackend.terminalRunning?"Send":"Run";onClicked:{if(workspaceBackend.terminalRunning)workspaceBackend.sendInput(commandLine.text);else win.runTerminal(commandLine.text);commandLine.clear()}}
      }
     }
    }
   }
  }
  Rectangle{visible:workspaceBackend.preferences["appearance/statusBar"];Layout.fillWidth:true;Layout.preferredHeight:visible?25:0;color:theme.shell
   RowLayout{anchors.fill:parent;anchors.leftMargin:12;anchors.rightMargin:12
    Label{text:workspaceBackend.message;color:theme.secondaryText;font.pixelSize:10;elide:Text.ElideMiddle;Layout.fillWidth:true}
    Label{text:"DWCODE / DESKTOP PREVIEW";color:theme.cyan;font.pixelSize:9;font.letterSpacing:1}
   }
  }
 }
 Rectangle{id:paletteOverlay;anchors.fill:parent;z:20;visible:win.paletteVisible;color:"#aa05080d"
  onVisibleChanged:if(visible){commandInput.text="";commandInput.forceActiveFocus()}
  MouseArea{anchors.fill:parent;onClicked:win.paletteVisible=false}
  Rectangle{width:620;height:420;anchors.horizontalCenter:parent.horizontalCenter;anchors.top:parent.top;anchors.topMargin:82;color:theme.elevated;border.color:theme.cyan;radius:8
   ColumnLayout{anchors.fill:parent;anchors.margins:15
    TextField{id:commandInput;Layout.fillWidth:true;placeholderText:"Type a command…";onAccepted:{for(let c of win.commands)if(c.title.toLowerCase().includes(text.toLowerCase())){win.activate(c.action);break}}}
    ListView{Layout.fillWidth:true;Layout.fillHeight:true;clip:true;model:win.commands
     delegate:ItemDelegate{required property var modelData;width:ListView.view.width;height:modelData.title.toLowerCase().includes(commandInput.text.toLowerCase())?39:0;visible:height>0;text:modelData.title+"    "+modelData.shortcut;onClicked:win.activate(modelData.action)}
    }
    Label{text:"DWCODE / KEYBOARD FIRST";font.pixelSize:10;color:theme.mutedText}
   }
  }
 }
}
