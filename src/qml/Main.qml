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
 property var commands: [
  { title: "Open Folder", shortcut: "Ctrl+O", action: "folder" },
  { title: "Save File", shortcut: "Ctrl+S", action: "save" },
  { title: "Save File As", shortcut: "Ctrl+Shift+S", action: "saveas" },
  { title: "Toggle File Explorer", shortcut: "Ctrl+B", action: "explorer" },
  { title: "Toggle Terminal", shortcut: "Ctrl+J", action: "terminal" },
  { title: "Split Editor", shortcut: "Ctrl+\\", action: "split" },
  { title: "Connect Language Server", shortcut: "", action: "lsp" },
  { title: "Toggle Helix Mode", shortcut: "Ctrl+Shift+M", action: "modal" }
 ]
 Theme { id: theme }
 palette.window: theme.background; palette.windowText: theme.foreground; palette.base: theme.canvas
 palette.text: theme.foreground; palette.button: theme.elevated; palette.buttonText: theme.foreground
 palette.highlight: theme.cyan; palette.highlightedText: theme.background
 function save(){editor.runJavaScript("window.dwcodeSave && window.dwcodeSave()")}
 function chooseFolder(){if(workspaceBackend.hasUnsaved()){pendingAction="folder";unsaved.open()}else folderDialog.open()}
 function activate(action){
  paletteVisible=false
  if(action==="folder")chooseFolder()
  else if(action==="save")save()
  else if(action==="saveas")saveDialog.open()
  else if(action==="explorer")explorerVisible=!explorerVisible
  else if(action==="terminal"){bottomVisible=!bottomVisible;activeBottomTab="TERMINAL"}
  else if(action==="split")splitVisible=!splitVisible
  else if(action==="lsp")workspaceBackend.startLanguageServer()
  else if(action==="modal"){modalMode=!modalMode;editor.runJavaScript("window.dwcodeSetMode("+modalMode+")");splitEditor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode("+modalMode+")")}
 }
 FolderDialog {id:folderDialog;title:"Open DWCode workspace";onAccepted:workspaceBackend.openWorkspace(selectedFolder.toString())}
 FileDialog {id:saveDialog;title:"Save file as (inside workspace)";fileMode:FileDialog.SaveFile;onAccepted:workspaceBackend.saveAs(selectedFile.toString())}
 Dialog {id:unsaved;title:"Unsaved files";anchors.centerIn:parent;modal:true;standardButtons:Dialog.Discard|Dialog.Cancel
  Label{text:"You have unsaved changes. Discard them to continue?";color:theme.foreground}
  onDiscarded:{workspaceBackend.discardChanges();if(pendingAction==="close"){closingApproved=true;win.close()}else folderDialog.open()}
 }
 onClosing:function(event){if(!closingApproved&&workspaceBackend.hasUnsaved()){event.accepted=false;pendingAction="close";unsaved.open()}}
 WebChannel {id:channel;Component.onCompleted:registerObject("native",workspaceBackend)}
 WebChannel {id:splitChannel;Component.onCompleted:registerObject("native",workspaceBackend)}
 Component.onCompleted:{if(workspaceBackend.files.length>0){for(let i=0;i<workspaceBackend.files.length;i++){let f=workspaceBackend.files[i];if(f.kind==="file"){workspaceBackend.openFile(f.path);break}}}}
 Shortcut {sequence:"Ctrl+O";onActivated:chooseFolder()}
 Shortcut {sequence:"Ctrl+S";onActivated:save()}
 Shortcut {sequence:"Ctrl+Shift+S";onActivated:saveDialog.open()}
 Shortcut {sequence:"Ctrl+P";onActivated:win.paletteVisible=true}
 Shortcut {sequence:"Ctrl+Shift+P";onActivated:win.paletteVisible=true}
 Shortcut {sequence:"Ctrl+B";onActivated:win.explorerVisible=!win.explorerVisible}
 Shortcut {sequence:"Ctrl+J";onActivated:activate("terminal")}
 Shortcut {sequence:"Ctrl+\\";onActivated:win.splitVisible=!win.splitVisible}
 Shortcut {sequence:"Ctrl+Shift+M";onActivated:activate("modal")}
 Shortcut {sequence:"Escape";enabled:win.paletteVisible;onActivated:win.paletteVisible=false}
 ColumnLayout {
  anchors.fill:parent;spacing:0
  Rectangle {
   Layout.fillWidth:true;Layout.preferredHeight:56;color:theme.shell;border.color:theme.border
   RowLayout {anchors.fill:parent;anchors.margins:10;spacing:12
    Image {source:"qrc:/qml/dwcode-mark.svg";Layout.preferredWidth:42;Layout.preferredHeight:32;fillMode:Image.PreserveAspectFit}
    Label{text:"DWCODE";font.bold:true;font.letterSpacing:2;color:theme.foreground}
    Button{text:"Open folder";onClicked:chooseFolder()}
    Button{text:workspaceBackend.dirty?"Save ●":"Save";enabled:workspaceBackend.activePath!=="";onClicked:save()}
    Button{text:"Commands ⌘";onClicked:win.paletteVisible=true}
    Item{Layout.fillWidth:true}
    Label{text:"CYBERCORE";color:theme.mutedText;font.pixelSize:9;font.letterSpacing:1}
    ComboBox {id:themePicker;Layout.preferredWidth:210;model:workspaceBackend.themes;textRole:"name";valueRole:"id"
     currentIndex:{for(let i=0;i<workspaceBackend.themes.length;i++)if(workspaceBackend.themes[i].id===workspaceBackend.theme.id)return i;return 0}
     onActivated:workspaceBackend.setTheme(currentValue)
     popup.height:Math.min(420,themePicker.count*36+16)
    }
    Button{text:workspaceBackend.lspReady?"LSP connected":"Connect LSP";onClicked:workspaceBackend.startLanguageServer()}
   }
  }
  RowLayout {
   Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
   Rectangle {Layout.preferredWidth:235;Layout.fillHeight:true;visible:win.explorerVisible;color:theme.sidebar;border.color:theme.border
    ColumnLayout {anchors.fill:parent;spacing:0
     RowLayout {Layout.fillWidth:true;Layout.margins:12
      Label{text:"EXPLORER";color:theme.secondaryText;font.pixelSize:10;font.letterSpacing:1}
      Item{Layout.fillWidth:true}ToolButton{text:"↻";onClicked:workspaceBackend.refreshFiles()}
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
    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:38;color:theme.shell;border.color:theme.border
     RowLayout {anchors.fill:parent;spacing:1
      Flickable {Layout.fillWidth:true;Layout.fillHeight:true;contentWidth:tabs.width;clip:true
       Row{id:tabs;height:parent.height
        Repeater{model:workspaceBackend.documents;delegate:Button{required property var modelData;height:38;text:(modelData.dirty?"● ":"")+modelData.name;flat:true;highlighted:workspaceBackend.activePath===modelData.path;onClicked:workspaceBackend.openFile(modelData.path)}}
       }
      }
      ToolButton{text:win.splitVisible?"Single view":"Split view";onClicked:win.splitVisible=!win.splitVisible}
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
       TextArea{id:consoleText;readOnly:true;selectByMouse:true;wrapMode:TextEdit.Wrap;color:theme.secondaryText;font.family:"monospace";font.pixelSize:11;background:null
        text:win.activeBottomTab==="TERMINAL"?workspaceBackend.terminalOutput:workspaceBackend.message+"\n\n"+workspaceBackend.lspStatus
        onTextChanged:if(win.activeBottomTab==="TERMINAL")cursorPosition=length
       }
      }
      ListView{Layout.fillWidth:true;Layout.fillHeight:true;visible:win.activeBottomTab==="PROBLEMS";clip:true;model:workspaceBackend.problems
       delegate:ItemDelegate{required property var modelData;width:ListView.view.width;text:modelData.path.split('/').pop()+":"+(modelData.range.start.line+1)+"  "+modelData.message;font.pixelSize:11;onClicked:workspaceBackend.openFile(modelData.path)}
      }
      RowLayout{Layout.fillWidth:true;Layout.margins:8;visible:win.activeBottomTab==="TERMINAL"
       Label{text:workspaceBackend.terminalRunning?"stdin ›":"$";color:theme.cyan}
       TextField{id:commandLine;Layout.fillWidth:true;placeholderText:workspaceBackend.terminalRunning?"Send input to running command…":"Run a command in the workspace…";font.family:"monospace";font.pixelSize:11
        onAccepted:{if(workspaceBackend.terminalRunning)workspaceBackend.sendInput(text);else workspaceBackend.runCommand(text);text=""}
       }
       Button{text:workspaceBackend.terminalRunning?"Send":"Run";onClicked:{if(workspaceBackend.terminalRunning)workspaceBackend.sendInput(commandLine.text);else workspaceBackend.runCommand(commandLine.text);commandLine.clear()}}
      }
     }
    }
   }
  }
  Rectangle{Layout.fillWidth:true;Layout.preferredHeight:25;color:theme.shell
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
