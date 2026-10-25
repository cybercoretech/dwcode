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
 function save(){editor.runJavaScript("window.dwcodeSave && window.dwcodeSave()")}
 function chooseFolder(){if(workspaceBackend.hasUnsaved()){pendingAction="folder";unsaved.open()}else folderDialog.open()}
 function activate(action){
  paletteVisible=false
  if(action==="folder")chooseFolder()
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
  else if(action==="undo")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','undo',{})")
  else if(action==="redo")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','redo',{})")
  else if(action==="selectall")editor.runJavaScript("window.dwcodeEditor && window.dwcodeEditor.trigger('dwcode','editor.action.selectAll',{})")
  else if(action==="modal"){modalMode=!modalMode;editor.runJavaScript("window.dwcodeSetMode("+modalMode+")");splitEditor.runJavaScript("window.dwcodeSetMode && window.dwcodeSetMode("+modalMode+")")}
 }
 FolderDialog {id:folderDialog;objectName:"folderDialog";title:"Open DWCode workspace";onAccepted:workspaceBackend.openWorkspace(selectedFolder.toString())}
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
 Shortcut {sequence:"Ctrl+Alt+M";onActivated:activate("modal")}
 Shortcut {sequence:"Escape";enabled:win.paletteVisible;onActivated:win.paletteVisible=false}
 ColumnLayout {
  anchors.fill:parent;spacing:0
  Rectangle {
   Layout.fillWidth:true;Layout.preferredHeight:64;color:theme.shell;border.color:theme.border
   ColumnLayout {anchors.fill:parent;spacing:0
    RowLayout {Layout.fillWidth:true;Layout.preferredHeight:31;Layout.leftMargin:10;Layout.rightMargin:10;spacing:10
     Image {source:"qrc:/qml/dwcode-icon.png";Layout.preferredHeight:18;Layout.preferredWidth:18;fillMode:Image.PreserveAspectFit;smooth:true;mipmap:true;Accessible.name:"DWCode"}
     MenuBar {id:applicationMenuBar;objectName:"applicationMenuBar";Layout.fillWidth:true;Layout.preferredHeight:31
      background:Rectangle{color:"transparent"}
      Menu {title:"File"
       MenuItem{text:"Open Folder…";onTriggered:chooseFolder()}
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
  RowLayout {
   Layout.fillWidth:true;Layout.fillHeight:true;spacing:0
   Rectangle {Layout.preferredWidth:235;Layout.fillHeight:true;visible:win.explorerVisible;color:theme.sidebar;border.color:theme.border
    ColumnLayout {anchors.fill:parent;spacing:0
     RowLayout {Layout.fillWidth:true;Layout.margins:12
      Label{text:"EXPLORER";color:theme.secondaryText;font.pixelSize:10;font.letterSpacing:1}
      Item{Layout.fillWidth:true}
      Button{text:"Open Folder";onClicked:chooseFolder()}
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
    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:38;color:theme.shell;border.color:theme.border
     RowLayout {anchors.fill:parent;spacing:1
      Flickable {Layout.fillWidth:true;Layout.fillHeight:true;contentWidth:tabs.width;clip:true
       Row{id:tabs;height:parent.height
        Repeater{model:workspaceBackend.documents;delegate:Button{id:tabButton;required property var modelData;property bool activeTab:workspaceBackend.activePath===modelData.path;height:38;leftPadding:15;rightPadding:15;text:(modelData.dirty?"● ":"")+modelData.name;flat:true;highlighted:activeTab;onClicked:workspaceBackend.openFile(modelData.path)
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
