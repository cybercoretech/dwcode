#include "workspacebackend.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QSettings>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QTimer>
#include <QCoreApplication>
#include <QRegularExpression>
#include <functional>
#include <signal.h>
#include <unistd.h>

namespace {
constexpr qint64 MaxFile=8*1024*1024;
QByteArray digest(const QByteArray &data){return QCryptographicHash::hash(data,QCryptographicHash::Sha256);}
QString localPath(const QString &path){return path.startsWith("file:")?QUrl(path).toLocalFile():path;}
QString language(const QString &path){
 const auto ext=QFileInfo(path).suffix().toLower();
 static const QMap<QString,QString> ids={{"ts","typescript"},{"tsx","typescriptreact"},{"js","javascript"},{"mjs","javascript"},{"jsx","javascriptreact"},{"rs","rust"},{"cpp","cpp"},{"h","cpp"},{"hpp","cpp"},{"cc","cpp"},{"c","c"},{"py","python"},{"json","json"},{"css","css"},{"html","html"},{"md","markdown"},{"qml","qml"},{"sh","shell"},{"toml","ini"},{"yaml","yaml"},{"yml","yaml"}};
 return ids.value(ext,"plaintext");
}
void terminateGroup(QProcess &process){
 if(process.state()==QProcess::NotRunning)return;
 const auto pid=process.processId();
 if(pid>0)::kill(-pid,SIGTERM);
 if(!process.waitForFinished(600)){if(pid>0)::kill(-pid,SIGKILL);process.kill();process.waitForFinished(600);}
}
}
WorkspaceBackend::WorkspaceBackend(QObject *parent):QObject(parent){
 m_terminal.setChildProcessModifier([]{::setsid();});
 m_terminal.setProcessChannelMode(QProcess::MergedChannels);
 connect(&m_terminal,&QProcess::readyReadStandardOutput,this,[this]{appendTerminal(QString::fromUtf8(m_terminal.readAllStandardOutput()));});
 connect(&m_terminal,&QProcess::started,this,&WorkspaceBackend::terminalChanged);
 connect(&m_terminal,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){appendTerminal(QString("\n[exit %1]\n").arg(code));emit terminalChanged();});
 connect(&m_terminal,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){report(m_terminal.errorString());emit terminalChanged();});
 m_lsp.setChildProcessModifier([]{::setsid();});
 connect(&m_lsp,&QProcess::readyReadStandardOutput,this,&WorkspaceBackend::readLsp);
 connect(&m_lsp,&QProcess::readyReadStandardError,this,[this]{m_lsp.readAllStandardError();});
 connect(&m_lsp,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){m_lspStatus=m_lsp.errorString();m_lspReady=false;emit lspChanged();});
 connect(&m_lsp,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){m_lspReady=false;m_lspStatus=QString("Stopped (%1)").arg(code);for(int id:std::as_const(m_pending))emit languageReply(id,QVariant(),"Language server stopped");m_pending.clear();emit lspChanged();});
 QFile themesFile(":/data/themes.json");
 if(themesFile.open(QIODevice::ReadOnly))m_themes=QJsonDocument::fromJson(themesFile.readAll()).array().toVariantList();
 setTheme(QSettings().value("theme","cyberpunk-chrome").toString());
}
WorkspaceBackend::~WorkspaceBackend(){terminateGroup(m_terminal);terminateGroup(m_lsp);}
void WorkspaceBackend::report(const QString &text){m_message=text;emit messageChanged();}
bool WorkspaceBackend::dirty() const{return m_documents.contains(m_active)&&m_documents.value(m_active).dirty;}
bool WorkspaceBackend::hasUnsaved() const{for(const auto &d:m_documents)if(d.dirty)return true;return false;}
void WorkspaceBackend::discardChanges(){m_documents.clear();m_active.clear();emit documentStateChanged();}
QVariantList WorkspaceBackend::documents() const{QVariantList out;for(const auto &doc:m_documents)out.append(describe(doc));return out;}
QVariantMap WorkspaceBackend::describe(const Document &doc)const{return {{"path",doc.path},{"name",QFileInfo(doc.path).fileName()},{"text",doc.text},{"language",doc.language},{"uri",QUrl::fromLocalFile(doc.path).toString()},{"version",doc.version},{"dirty",doc.dirty}};}
QVariantMap WorkspaceBackend::activeDocument()const{return m_documents.contains(m_active)?describe(m_documents.value(m_active)):QVariantMap();}
QString WorkspaceBackend::resolve(const QString &input,bool newFile)const{
 if(m_root.isEmpty())return {};
 QString path=localPath(input);if(QDir::isRelativePath(path))path=QDir(m_root).filePath(path);
 QFileInfo info(path);QString result=info.canonicalFilePath();
 if(result.isEmpty()&&newFile&&!info.isSymLink()){const auto parent=info.dir().canonicalPath();if(!parent.isEmpty())result=parent+"/"+info.fileName();}
 if(result.isEmpty()||!result.startsWith(m_root+"/"))return {};
 return result;
}
bool WorkspaceBackend::openWorkspace(const QString &input){
 if(hasUnsaved()){report("Save or discard unsaved files before changing folders.");return false;}
 const auto path=QFileInfo(localPath(input)).canonicalFilePath();
 if(path.isEmpty()||!QFileInfo(path).isDir()){report("Choose an existing project folder.");return false;}
 stopCommand();stopLanguageServer();m_documents.clear();m_active.clear();m_root=path;refreshFiles();emit documentStateChanged();report("Workspace: "+path);return true;
}
void WorkspaceBackend::refreshFiles(){
 m_files.clear();int count=0;
 const QSet<QString> skipped={".git","node_modules","target","build","dist",".cache",".venv","__pycache__"};
 std::function<void(const QDir&,int)> scan=[&](const QDir &dir,int depth){
  if(depth>10||count>=3000)return;
  const auto entries=dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot,QDir::DirsFirst|QDir::Name);
  for(const auto &info:entries){if(count>=3000)break;if(info.isSymLink()||skipped.contains(info.fileName()))continue;
   const auto rel=QDir(m_root).relativeFilePath(info.filePath());
   m_files.append(QVariantMap{{"name",info.fileName()},{"path",rel},{"depth",depth},{"kind",info.isDir()?"folder":"file"},{"language",language(info.fileName())}});++count;
   if(info.isDir())scan(QDir(info.filePath()),depth+1);
  }
 };
 if(!m_root.isEmpty())scan(QDir(m_root),0);
 emit workspaceChanged();
 if(count>=3000)report("Explorer limited to 3,000 entries. Build and dependency folders are excluded.");
}
QVariantMap WorkspaceBackend::openFile(const QString &input){
 const auto path=resolve(input);QFile file(path);
 if(path.isEmpty()||!QFileInfo(path).isFile()){report("File must be inside the current workspace.");return {};}
 if(!m_documents.contains(path)){
  if(file.size()>MaxFile||!file.open(QIODevice::ReadOnly)){report("Unable to read file (8 MiB text limit).");return {};}
  const auto bytes=file.readAll();QStringDecoder decoder(QStringDecoder::Utf8);QString text=decoder(bytes);
  if(decoder.hasError()||bytes.contains('\0')){report("This file is not UTF-8 text.");return {};}
  m_documents.insert(path,Document{path,text,language(path),digest(bytes),1,false});
 }
 m_active=path;openInLsp(m_documents[path]);emit documentStateChanged();const auto doc=activeDocument();emit documentActivated(doc);return doc;
}
bool WorkspaceBackend::updateDocument(const QString &path,const QString &text){
 if(!m_documents.contains(path)||text.toUtf8().size()>MaxFile)return false;
 auto &doc=m_documents[path];if(doc.text==text)return true;doc.text=text;++doc.version;doc.dirty=digest(text.toUtf8())!=doc.diskHash;
 if(m_lspReady&&supports(doc)){
  if(!m_lspOpened.contains(path))openInLsp(doc);
  else notifyLsp("textDocument/didChange",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(path).toString()},{"version",doc.version}}},{"contentChanges",QJsonArray{QJsonObject{{"text",text}}}}});
 }
 emit documentStateChanged();emit documentUpdated(describe(doc));return true;
}
bool WorkspaceBackend::writeDocument(Document &doc,const QString &target,bool overwrite){
 if(target.isEmpty()){report("Save destination must be inside the workspace.");return false;}
 if(QFileInfo::exists(target)){
  if(!overwrite){report("Save As will not overwrite an existing file. Choose another name.");return false;}
  QFile old(target);if(!old.open(QIODevice::ReadOnly)||digest(old.readAll())!=doc.diskHash){report("Save blocked: the file changed on disk. Use Save As to preserve your edits.");return false;}
 }
 QSaveFile out(target);const auto bytes=doc.text.toUtf8();
 if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit()){report("Save failed: "+out.errorString());return false;}
 doc.diskHash=digest(bytes);doc.dirty=false;report("Saved "+QDir(m_root).relativeFilePath(target));emit documentStateChanged();return true;
}
bool WorkspaceBackend::saveDocument(const QString &path){
 if(!m_documents.contains(path)){report("Open a workspace file before saving.");return false;}
 if(resolve(path)!=path){report("Save blocked: file path no longer belongs to this workspace.");return false;}
 auto &doc=m_documents[path];if(!writeDocument(doc,path,true))return false;
 if(m_lspReady&&supports(doc))notifyLsp("textDocument/didSave",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(path).toString()}}}});
 return true;
}
bool WorkspaceBackend::saveAs(const QString &url){
 if(!m_documents.contains(m_active)){report("Open a file first.");return false;}
 const auto target=resolve(url,true);if(target==m_active)return saveDocument(m_active);
 auto doc=m_documents[m_active];if(!writeDocument(doc,target,false))return false;
 doc.path=target;doc.language=language(target);m_documents.insert(target,doc);m_active=target;refreshFiles();emit documentStateChanged();emit documentActivated(describe(doc));openInLsp(doc);return true;
}
void WorkspaceBackend::appendTerminal(const QString &text){m_terminalOutput+=text;m_terminalOutput=m_terminalOutput.right(200000);emit terminalChanged();}
bool WorkspaceBackend::runCommand(const QString &command){
 if(m_root.isEmpty()){report("Open a folder before running commands.");return false;}
 if(terminalRunning()||command.trimmed().isEmpty())return false;
 appendTerminal("\n"+m_root+" $ "+command+"\n");m_terminal.setWorkingDirectory(m_root);
 auto env=QProcessEnvironment::systemEnvironment();env.insert("TERM","dumb");env.insert("NO_COLOR","1");m_terminal.setProcessEnvironment(env);
 m_terminal.start("/bin/bash",{"--noprofile","--norc","-c",command});return true;
}
void WorkspaceBackend::sendInput(const QString &text){if(terminalRunning())m_terminal.write((text+"\n").toUtf8());}
void WorkspaceBackend::stopCommand(){terminateGroup(m_terminal);emit terminalChanged();}
void WorkspaceBackend::clearTerminal(){m_terminalOutput.clear();emit terminalChanged();}
void WorkspaceBackend::setTheme(const QString &id){
 for(const auto &value:m_themes){const auto theme=value.toMap();if(theme.value("id")==id){m_theme=theme;QSettings().setValue("theme",id);emit themeChanged();return;}}
 if(!m_themes.isEmpty()){m_theme=m_themes.first().toMap();emit themeChanged();}
}
void WorkspaceBackend::sendLsp(const QJsonObject &object){
 if(m_lsp.state()!=QProcess::Running)return;
 const auto body=QJsonDocument(object).toJson(QJsonDocument::Compact);m_lsp.write("Content-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);
}
void WorkspaceBackend::notifyLsp(const QString &method,const QJsonObject &params){sendLsp({{"jsonrpc","2.0"},{"method",method},{"params",params}});}
bool WorkspaceBackend::supports(const Document &doc)const{
 if(m_lspFamily=="typescript")return doc.language.startsWith("typescript")||doc.language.startsWith("javascript");
 if(m_lspFamily=="cpp")return doc.language=="cpp"||doc.language=="c";
 return doc.language==m_lspFamily;
}
void WorkspaceBackend::openInLsp(const Document &doc){
 if(!m_lspReady||!supports(doc)||m_lspOpened.contains(doc.path))return;
 m_lspOpened.insert(doc.path);notifyLsp("textDocument/didOpen",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(doc.path).toString()},{"languageId",doc.language},{"version",doc.version},{"text",doc.text}}}});
}
void WorkspaceBackend::startLanguageServer(){
 if(m_active.isEmpty()){report("Open a supported source file first.");return;}
 stopLanguageServer();QString executable;QStringList args;const auto lang=m_documents[m_active].language;
 if(lang.startsWith("typescript")||lang.startsWith("javascript")){m_lspFamily="typescript";executable=QStringLiteral(DWCODE_SOURCE_ROOT)+"/node_modules/.bin/typescript-language-server";if(!QFileInfo::exists(executable))executable=QStandardPaths::findExecutable("typescript-language-server");args={"--stdio"};}
 else if(lang=="cpp"||lang=="c"){m_lspFamily="cpp";executable=QStandardPaths::findExecutable("clangd");}
 else if(lang=="rust"){m_lspFamily="rust";executable=QStandardPaths::findExecutable("rust-analyzer");}
 else if(lang=="python"){m_lspFamily="python";executable=QStandardPaths::findExecutable("pyright-langserver");args={"--stdio"};}
 else {m_lspStatus="No server configured for "+lang;emit lspChanged();return;}
 if(executable.isEmpty()){m_lspStatus="Server not installed for "+lang;emit lspChanged();return;}
 m_lsp.setWorkingDirectory(m_root);m_lsp.start(executable,args);
 if(!m_lsp.waitForStarted(3000)){m_lspStatus=m_lsp.errorString();emit lspChanged();return;}
 m_lspStatus="Connecting: "+QFileInfo(executable).fileName();emit lspChanged();
 QJsonObject textCaps{{"hover",QJsonObject{{"contentFormat",QJsonArray{"plaintext","markdown"}}}},{"completion",QJsonObject{{"completionItem",QJsonObject{{"snippetSupport",false}}}}},{"publishDiagnostics",QJsonObject{{"relatedInformation",true}}}};
 sendLsp({{"jsonrpc","2.0"},{"id",1},{"method","initialize"},{"params",QJsonObject{{"processId",QCoreApplication::applicationPid()},{"rootUri",QUrl::fromLocalFile(m_root).toString()},{"capabilities",QJsonObject{{"textDocument",textCaps},{"general",QJsonObject{{"positionEncodings",QJsonArray{"utf-16"}}}}}},{"workspaceFolders",QJsonArray{QJsonObject{{"uri",QUrl::fromLocalFile(m_root).toString()},{"name",QFileInfo(m_root).fileName()}}}},{"initializationOptions",QJsonObject{{"hostInfo","DWCode"},{"tsserver",QJsonObject{{"path",QStringLiteral(DWCODE_SOURCE_ROOT)+"/node_modules/typescript/lib/tsserver.js"}}}}}}}});
 const auto startingPid=m_lsp.processId();
 QTimer::singleShot(15000,this,[this,startingPid]{if(m_lsp.processId()==startingPid&&m_lsp.state()==QProcess::Running&&!m_lspReady){stopLanguageServer();m_lspStatus="Initialization timed out";emit lspChanged();}});
}
void WorkspaceBackend::stopLanguageServer(){
 terminateGroup(m_lsp);m_lspReady=false;m_lspBuffer.clear();m_lspOpened.clear();m_diagnostics.clear();m_problems.clear();m_lspStatus="Disconnected";emit lspChanged();emit problemsChanged();
}
int WorkspaceBackend::languageRequest(const QString &method,const QVariantMap &params){
 if(!m_lspReady||!QSet<QString>{"textDocument/hover","textDocument/completion","textDocument/definition"}.contains(method))return -1;
 const auto path=QUrl(params.value("textDocument").toMap().value("uri").toString()).toLocalFile();if(!m_lspOpened.contains(path))return -1;
 const int id=++m_requestId;m_pending.insert(id);sendLsp({{"jsonrpc","2.0"},{"id",id},{"method",method},{"params",QJsonObject::fromVariantMap(params)}});
 QTimer::singleShot(10000,this,[this,id]{if(m_pending.remove(id))emit languageReply(id,QVariant(),"Language request timed out");});return id;
}
void WorkspaceBackend::readLsp(){
 m_lspBuffer+=m_lsp.readAllStandardOutput();
 while(true){const int end=m_lspBuffer.indexOf("\r\n\r\n");if(end<0)break;
  int length=-1;for(const auto &line:m_lspBuffer.left(end).split('\n'))if(line.trimmed().toLower().startsWith("content-length:"))length=line.mid(line.indexOf(':')+1).trimmed().toInt();
  if(length<0||length>32*1024*1024){stopLanguageServer();report("Invalid language server message.");return;}
  if(m_lspBuffer.size()<end+4+length)break;
  const auto body=m_lspBuffer.mid(end+4,length);m_lspBuffer.remove(0,end+4+length);handleLsp(QJsonDocument::fromJson(body).object());
 }
 if(m_lspBuffer.size()>32*1024*1024)stopLanguageServer();
}
void WorkspaceBackend::handleLsp(const QJsonObject &msg){
 const auto method=msg.value("method").toString();
 if(msg.contains("id")&&!method.isEmpty()){
  if(method=="workspace/configuration"){QJsonArray items;for(const auto &item:msg.value("params").toObject().value("items").toArray()){Q_UNUSED(item);items.append(QJsonObject{});}sendLsp({{"jsonrpc","2.0"},{"id",msg.value("id")},{"result",items}});}
  else if(method=="client/registerCapability"||method=="window/workDoneProgress/create")sendLsp({{"jsonrpc","2.0"},{"id",msg.value("id")},{"result",QJsonValue::Null}});
  else sendLsp({{"jsonrpc","2.0"},{"id",msg.value("id")},{"error",QJsonObject{{"code",-32601},{"message","Unsupported client method"}}}});
  return;
 }
 if(msg.contains("id")){
  const int id=msg.value("id").toInt();const QString error=msg.value("error").toObject().value("message").toString();
  if(id==1){if(!error.isEmpty()){m_lspStatus=error;emit lspChanged();return;}m_lspReady=true;m_lspStatus=m_lspFamily+" · connected";notifyLsp("initialized",{});for(const auto &doc:m_documents)openInLsp(doc);emit lspChanged();}
  else if(m_pending.remove(id))emit languageReply(id,msg.value("result").toVariant(),error);
 } else if(method=="textDocument/publishDiagnostics"){
  const auto params=msg.value("params").toObject();const auto uri=params.value("uri").toString();const auto diagnostics=params.value("diagnostics").toArray().toVariantList();
  if(!m_lspOpened.contains(QUrl(uri).toLocalFile()))return;
  m_diagnostics[uri]=diagnostics;m_problems.clear();for(auto it=m_diagnostics.cbegin();it!=m_diagnostics.cend();++it)for(const auto &d:it.value()){auto problem=d.toMap();problem["path"]=QUrl(it.key()).toLocalFile();m_problems.append(problem);}
  emit diagnosticsReceived(uri,diagnostics);emit problemsChanged();
 }
}
