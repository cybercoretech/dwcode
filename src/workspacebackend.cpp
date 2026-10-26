#include "workspacebackend.h"
#include "packloader.h"
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
#include <QStringList>
#include <QTimer>
#include <QCoreApplication>
#include <QRegularExpression>
#include <functional>
#include <signal.h>
#include <unistd.h>

namespace {
constexpr qint64 MaxFile=8*1024*1024;
constexpr int SessionVersion=1;
QByteArray digest(const QByteArray &data){return QCryptographicHash::hash(data,QCryptographicHash::Sha256);}
// Buffer files are named from the path so a restart can find them again.
QString bufferName(const QString &path){return QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(),QCryptographicHash::Sha1).toHex().left(24));}
QString localPath(const QString &path){return path.startsWith("file:")?QUrl(path).toLocalFile():path;}
QString language(const QString &path){
 const auto ext=QFileInfo(path).suffix().toLower();
 static const QMap<QString,QString> ids={{"ts","typescript"},{"tsx","typescriptreact"},{"js","javascript"},{"mjs","javascript"},{"jsx","javascriptreact"},{"rs","rust"},{"cpp","cpp"},{"h","cpp"},{"hpp","cpp"},{"cc","cpp"},{"c","c"},{"py","python"},{"json","json"},{"css","css"},{"html","html"},{"md","markdown"},{"qml","qml"},{"sh","shell"},{"toml","ini"},{"yaml","yaml"},{"yml","yaml"}};
 return ids.value(ext,"plaintext");
}
QVariantMap preferenceDefaults(){return {
 {"appearance/uiScale",100},{"appearance/density","comfortable"},{"appearance/sidebarPosition","left"},{"appearance/statusBar",true},
 {"editor/fontFamily","JetBrains Mono, Cascadia Code, monospace"},{"editor/fontSize",14},{"editor/fontLigatures",true},{"editor/lineHeight",23},{"editor/wordWrap","off"},{"editor/minimap",true},{"editor/lineNumbers","on"},{"editor/rulers",""},{"editor/renderWhitespace","none"},{"editor/bracketGuides",true},{"editor/currentLineHighlight",true},{"editor/smoothScrolling",true},
 {"editing/tabSize",2},{"editing/insertSpaces",true},{"editing/autoIndent",true},{"editing/autoClosingBrackets",true},{"editing/trimTrailingWhitespace",false},{"editing/insertFinalNewline",false},{"editing/lineEnding","auto"},
 {"files/autoSave","off"},{"files/autoSaveDelay",1000},{"files/excludes",""},{"files/restoreSession",true},
 {"terminal/shell","/bin/bash"},{"terminal/workingDirectory","workspace"},{"terminal/fontFamily","monospace"},{"terminal/fontSize",11},{"terminal/scrollback",200000},{"terminal/copyOnSelect",true},{"terminal/clearOnRun",false},{"terminal/environment",""},{"terminal/warnRisky",true},
 {"keyboard/modalDefault",false},{"keyboard/modalCursor","block"}
};}
void terminateGroup(QProcess &process){
 if(process.state()==QProcess::NotRunning)return;
 const auto pid=process.processId();
 if(pid>0)::kill(-pid,SIGTERM);
 if(!process.waitForFinished(600)){if(pid>0)::kill(-pid,SIGKILL);process.kill();process.waitForFinished(600);}
}
}
WorkspaceBackend::WorkspaceBackend(QObject *parent):QObject(parent){
 loadPreferences();
 m_refreshTimer.setSingleShot(true);m_refreshTimer.setInterval(150);
 connect(&m_refreshTimer,&QTimer::timeout,this,&WorkspaceBackend::refreshFiles);
 // Unsaved buffers are journaled a beat after each edit, so a crash loses at
 // most the tail of the last few hundred milliseconds of typing.
 m_sessionTimer.setSingleShot(true);m_sessionTimer.setInterval(400);
 connect(&m_sessionTimer,&QTimer::timeout,this,&WorkspaceBackend::writeSession);
 connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,[this](const QString &){scheduleRefresh();});
 connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,[this](const QString &path){checkExternalChange(path);scheduleRefresh();});
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
 if(themesFile.open(QIODevice::ReadOnly))m_builtinThemes=QJsonDocument::fromJson(themesFile.readAll()).array().toVariantList();
 loadPacks();
 rebuildThemes();
 setTheme(QSettings().value("theme","cyberpunk-chrome").toString());
 loadSession();
}
WorkspaceBackend::~WorkspaceBackend(){flushSession();terminateGroup(m_terminal);terminateGroup(m_lsp);}
void WorkspaceBackend::loadPreferences(){
 m_preferences=preferenceDefaults();QSettings settings;
 for(auto it=m_preferences.begin();it!=m_preferences.end();++it)it.value()=settings.value("settings/"+it.key(),it.value());
}
void WorkspaceBackend::report(const QString &text){m_message=text;emit messageChanged();}
bool WorkspaceBackend::dirty() const{return m_documents.contains(m_active)&&m_documents.value(m_active).dirty;}
bool WorkspaceBackend::hasUnsaved() const{for(const auto &d:m_documents)if(d.dirty)return true;return false;}
void WorkspaceBackend::discardChanges(){m_documents.clear();m_order.clear();m_active.clear();m_externalChanges.clear();watchWorkspace();emit documentStateChanged();flushSession();}
QVariantList WorkspaceBackend::documents() const{QVariantList out;for(const auto &path:orderedPaths())out.append(describe(m_documents.value(path)));return out;}
QStringList WorkspaceBackend::orderedPaths() const{
 QStringList out;QSet<QString> seen;
 for(const auto &path:m_order)if(m_documents.contains(path)&&!seen.contains(path)){out.append(path);seen.insert(path);}
 for(auto it=m_documents.cbegin();it!=m_documents.cend();++it)if(!seen.contains(it.key())){out.append(it.key());seen.insert(it.key());}
 return out;
}
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
 stopCommand();stopLanguageServer();m_documents.clear();m_order.clear();m_active.clear();m_externalChanges.clear();m_root=path;refreshFiles();emit documentStateChanged();report("Workspace: "+path);flushSession();return true;
}
void WorkspaceBackend::refreshFiles(){
 m_files.clear();int count=0;
 const QSet<QString> skipped={".git","node_modules","target","build","dist",".cache",".venv","__pycache__"};
 const auto excluded=m_preferences.value("files/excludes").toString().split(QRegularExpression("[;\\n]"),Qt::SkipEmptyParts);
 std::function<void(const QDir&,int)> scan=[&](const QDir &dir,int depth){
  if(depth>10||count>=3000)return;
  const auto entries=dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot,QDir::DirsFirst|QDir::Name);
  for(const auto &info:entries){if(count>=3000)break;
   const auto rel=QDir(m_root).relativeFilePath(info.filePath());
   bool ignored=info.isSymLink()||skipped.contains(info.fileName());for(const auto &pattern:excluded)ignored=ignored||QDir::match(pattern.trimmed(),info.fileName())||QDir::match(pattern.trimmed(),rel);
   if(ignored)continue;
   m_files.append(QVariantMap{{"name",info.fileName()},{"path",rel},{"depth",depth},{"kind",info.isDir()?"folder":"file"},{"language",language(info.fileName())}});++count;
   if(info.isDir())scan(QDir(info.filePath()),depth+1);
  }
 };
 if(!m_root.isEmpty())scan(QDir(m_root),0);
 watchWorkspace();
 emit workspaceChanged();
 if(count>=3000)report("Explorer limited to 3,000 entries. Build and dependency folders are excluded.");
}
void WorkspaceBackend::watchWorkspace(){
 const auto oldFiles=m_watcher.files(),oldDirectories=m_watcher.directories();
 if(!oldFiles.isEmpty())m_watcher.removePaths(oldFiles);
 if(!oldDirectories.isEmpty())m_watcher.removePaths(oldDirectories);
 if(m_root.isEmpty())return;
 QStringList directories{m_root},files;
 for(const auto &entry:m_files){const auto path=QDir(m_root).filePath(entry.toMap().value("path").toString());if(entry.toMap().value("kind")=="folder"&&QFileInfo(path).isDir())directories.append(path);}
 for(auto it=m_documents.cbegin();it!=m_documents.cend();++it)if(QFileInfo::exists(it.key()))files.append(it.key());
 m_watcher.addPaths(directories);if(!files.isEmpty())m_watcher.addPaths(files);
}
void WorkspaceBackend::scheduleRefresh(){if(!m_refreshTimer.isActive())m_refreshTimer.start();}
void WorkspaceBackend::checkExternalChange(const QString &path){
 if(!m_documents.contains(path))return;
 const auto &doc=m_documents.value(path);QFile file(path);
 const bool deleted=!file.exists();QByteArray externalHash;
 if(!deleted){if(!file.open(QIODevice::ReadOnly))return;externalHash=digest(file.readAll());}
 if(!deleted&&externalHash==doc.diskHash)return;
 if(m_externalChanges.contains(path)&&m_externalChanges.value(path)==externalHash)return;
 m_externalChanges.insert(path,externalHash);
 const auto name=QFileInfo(path).fileName();
 report(deleted?name+" was deleted outside DWCode.":name+" changed outside DWCode.");
 emit documentExternalChange({{"path",path},{"name",name},{"dirty",doc.dirty},{"deleted",deleted}});
}
bool WorkspaceBackend::createFile(const QString &input){
 const auto path=resolve(input,true);
 if(path.isEmpty()||QFileInfo(path).fileName().isEmpty()){report("Choose a new file name inside the workspace.");return false;}
 if(QFileInfo::exists(path)||m_documents.contains(path)){report("A file with that name already exists.");return false;}
 m_documents.insert(path,Document{path,"",language(path),digest({}),1,true,QString()});m_order.append(path);m_externalChanges.remove(path);m_active=path;
 report("New unsaved file: "+QDir(m_root).relativeFilePath(path));emit documentStateChanged();emit documentActivated(activeDocument());flushSession();return true;
}
QVariantMap WorkspaceBackend::openFile(const QString &input){
 const auto path=resolve(input);QFile file(path);
 if(path.isEmpty()||!QFileInfo(path).isFile()){report("File must be inside the current workspace.");return {};}
 if(!m_documents.contains(path)){
  if(file.size()>MaxFile||!file.open(QIODevice::ReadOnly)){report("Unable to read file (8 MiB text limit).");return {};}
  const auto bytes=file.readAll();QStringDecoder decoder(QStringDecoder::Utf8);QString text=decoder(bytes);
  if(decoder.hasError()||bytes.contains('\0')){report("This file is not UTF-8 text.");return {};}
  m_documents.insert(path,Document{path,text,language(path),digest(bytes),1,false,QString()});m_order.append(path);
 }
 m_externalChanges.remove(path);watchWorkspace();m_active=path;openInLsp(m_documents[path]);emit documentStateChanged();const auto doc=activeDocument();emit documentActivated(doc);flushSession();return doc;
}
bool WorkspaceBackend::updateDocument(const QString &path,const QString &text){
 if(!m_documents.contains(path)||text.toUtf8().size()>MaxFile)return false;
 auto &doc=m_documents[path];if(doc.text==text)return true;doc.text=text;++doc.version;doc.dirty=digest(text.toUtf8())!=doc.diskHash;
 scheduleSessionWrite();
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
 const auto before=doc.text;doc.text=preparedForSave(doc.text);
 if(doc.text!=before){++doc.version;if(m_lspReady&&supports(doc))notifyLsp("textDocument/didChange",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(doc.path).toString()},{"version",doc.version}}},{"contentChanges",QJsonArray{QJsonObject{{"text",doc.text}}}}});}
 QSaveFile out(target);const auto bytes=doc.text.toUtf8();
 if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit()){report("Save failed: "+out.errorString());return false;}
 doc.diskHash=digest(bytes);doc.dirty=false;report("Saved "+QDir(m_root).relativeFilePath(target));emit documentStateChanged();flushSession();return true;
}
bool WorkspaceBackend::saveDocument(const QString &path){
 if(!m_documents.contains(path)){report("Open a workspace file before saving.");return false;}
 if(resolve(path,true)!=path){report("Save blocked: file path no longer belongs to this workspace.");return false;}
 auto &doc=m_documents[path];if(!writeDocument(doc,path,true))return false;
 m_externalChanges.remove(path);watchWorkspace();
 if(m_lspReady&&supports(doc))notifyLsp("textDocument/didSave",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(path).toString()}}}});
 emit documentUpdated(describe(doc));
 return true;
}
bool WorkspaceBackend::reloadDocument(const QString &input){
 const auto path=resolve(input);QFile file(path);
 if(path.isEmpty()||!m_documents.contains(path)||!file.open(QIODevice::ReadOnly)){report("The changed file is no longer available to reload.");return false;}
 const auto bytes=file.readAll();QStringDecoder decoder(QStringDecoder::Utf8);const QString text=decoder(bytes);
 if(decoder.hasError()||bytes.contains('\0')||bytes.size()>MaxFile){report("The changed file is not supported UTF-8 text.");return false;}
 auto &doc=m_documents[path];doc.text=text;doc.diskHash=digest(bytes);++doc.version;doc.dirty=false;m_externalChanges.remove(path);
 if(m_lspReady&&supports(doc))notifyLsp("textDocument/didChange",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(path).toString()},{"version",doc.version}}},{"contentChanges",QJsonArray{QJsonObject{{"text",doc.text}}}}});
 report("Reloaded "+QDir(m_root).relativeFilePath(path));emit documentStateChanged();emit documentUpdated(describe(doc));flushSession();return true;
}
bool WorkspaceBackend::saveAs(const QString &url){
 if(!m_documents.contains(m_active)){report("Open a file first.");return false;}
 const auto target=resolve(url,true);if(target==m_active)return saveDocument(m_active);
 const auto previous=m_active;
 auto doc=m_documents[previous];if(!writeDocument(doc,target,false))return false;
 doc.path=target;doc.language=language(target);m_documents.remove(previous);m_documents.insert(target,doc);
 const int at=m_order.indexOf(previous);if(at>=0)m_order.replace(at,target);else m_order.append(target);
 if(m_lspReady&&m_lspOpened.remove(previous))notifyLsp("textDocument/didClose",{{"textDocument",QJsonObject{{"uri",QUrl::fromLocalFile(previous).toString()}}}});
 m_externalChanges.remove(previous);m_externalChanges.remove(target);m_active=target;refreshFiles();emit documentStateChanged();emit documentActivated(describe(doc));openInLsp(doc);flushSession();return true;
}
void WorkspaceBackend::appendTerminal(const QString &text){m_terminalOutput+=text;m_terminalOutput=m_terminalOutput.right(m_preferences.value("terminal/scrollback",200000).toInt());emit terminalChanged();}
bool WorkspaceBackend::runCommand(const QString &command){
 if(m_root.isEmpty()){report("Open a folder before running commands.");return false;}
 if(terminalRunning()||command.trimmed().isEmpty())return false;
 if(m_preferences.value("terminal/clearOnRun").toBool())m_terminalOutput.clear();
 appendTerminal("\n"+m_root+" $ "+command+"\n");m_terminal.setWorkingDirectory(m_preferences.value("terminal/workingDirectory").toString()=="home"?QDir::homePath():m_root);
 auto env=QProcessEnvironment::systemEnvironment();env.insert("TERM","dumb");env.insert("NO_COLOR","1");m_terminal.setProcessEnvironment(env);
 for(const auto &line:m_preferences.value("terminal/environment").toString().split('\n')){const int equals=line.indexOf('=');if(equals>0)env.insert(line.left(equals).trimmed(),line.mid(equals+1));}
 m_terminal.setProcessEnvironment(env);const auto shell=m_preferences.value("terminal/shell").toString();
 m_terminal.start(shell,shell.endsWith("bash")?QStringList{"--noprofile","--norc","-c",command}:QStringList{"-c",command});return true;
}
void WorkspaceBackend::sendInput(const QString &text){if(terminalRunning())m_terminal.write((text+"\n").toUtf8());}
void WorkspaceBackend::stopCommand(){terminateGroup(m_terminal);emit terminalChanged();}
void WorkspaceBackend::clearTerminal(){m_terminalOutput.clear();emit terminalChanged();}
void WorkspaceBackend::setTheme(const QString &id){
 for(const auto &value:m_themes){const auto theme=value.toMap();if(theme.value("id")==id){m_theme=theme;QSettings().setValue("theme",id);emit themeChanged();return;}}
 if(!m_themes.isEmpty()){m_theme=m_themes.first().toMap();emit themeChanged();}
}
QString WorkspaceBackend::preparedForSave(const QString &input) const{
 QString text=input;text.replace("\r\n","\n");text.replace('\r','\n');
 if(m_preferences.value("editing/trimTrailingWhitespace").toBool())text.replace(QRegularExpression("[ \\t]+(?=\\n|$)"),"");
 if(m_preferences.value("editing/insertFinalNewline").toBool()&&!text.endsWith('\n'))text.append('\n');
 if(m_preferences.value("editing/lineEnding").toString()=="crlf")text.replace("\n","\r\n");
 return text;
}
void WorkspaceBackend::setPreference(const QString &key,const QVariant &value){
 const auto defaults=preferenceDefaults();if(!defaults.contains(key))return;
 QVariant normalized=value;
 if(key=="appearance/uiScale")normalized=qBound(80,value.toInt(),150);
 else if(key=="editor/fontSize")normalized=qBound(9,value.toInt(),32);
 else if(key=="editor/lineHeight")normalized=qBound(12,value.toInt(),48);
 else if(key=="editing/tabSize")normalized=(value.toInt()==8?8:value.toInt()==4?4:2);
 else if(key=="files/autoSaveDelay")normalized=qBound(250,value.toInt(),10000);
 else if(key=="terminal/fontSize")normalized=qBound(8,value.toInt(),28);
 else if(key=="terminal/scrollback")normalized=qBound(1000,value.toInt(),1000000);
 if(m_preferences.value(key)==normalized)return;
 m_preferences[key]=normalized;
 QSettings().setValue("settings/"+key,normalized);
 if(key=="files/excludes")refreshFiles();
 emit preferencesChanged(m_preferences);
}
void WorkspaceBackend::resetPreferences(){
 const auto defaults=preferenceDefaults();QSettings settings;for(auto it=defaults.cbegin();it!=defaults.cend();++it)settings.remove("settings/"+it.key());
 loadPreferences();setTheme("cyberpunk-chrome");emit preferencesChanged(m_preferences);
}
// Packs live beside the settings store so test isolation carries over, same as
// the session journal.
QString WorkspaceBackend::packDirectory(){return PackLoader::userDirectory();}
void WorkspaceBackend::loadPacks(){
 m_packs.clear();
 QSettings settings;
 for(const auto &pack:PackLoader::loadAll()){
  auto entry=PackLoader::toVariant(pack);
  // Packs that reached the directory are installed deliberately; enablement is
  // a user switch, and install-time consent arrives with the catalog (M5).
  entry.insert(QStringLiteral("enabled"),settings.value("settings/packs/"+pack.id+"/enabled",true).toBool());
  m_packs<<entry;
 }
}
void WorkspaceBackend::rebuildThemes(){
 m_themes=m_builtinThemes;
 m_packSnippets.clear();
 QSet<QString> seen;
 for(const auto &value:std::as_const(m_builtinThemes))seen.insert(value.toMap().value("id").toString());
 for(const auto &value:std::as_const(m_packs)){
  const auto pack=value.toMap();
  if(!pack.value("valid").toBool()||!pack.value("enabled").toBool())continue;
  for(const auto &themeValue:pack.value("themes").toList()){
   const auto theme=themeValue.toMap();
   const QString id=theme.value("id").toString();
   if(id.isEmpty()||seen.contains(id))continue;
   seen.insert(id);m_themes<<themeValue;
  }
  const auto snippets=pack.value("snippets").toMap();
  for(auto it=snippets.cbegin();it!=snippets.cend();++it){
   auto list=m_packSnippets.value(it.key()).toList();
   for(const auto &snippet:it.value().toList())list<<snippet;
   m_packSnippets.insert(it.key(),list);
  }
 }
 // The active theme may have left with the pack that provided it.
 if(!m_theme.isEmpty()&&!seen.contains(m_theme.value("id").toString()))
  setTheme(m_builtinThemes.isEmpty()?QString():m_builtinThemes.first().toMap().value("id").toString());
 emit themesChanged();
}
bool WorkspaceBackend::setPackEnabled(const QString &id,bool enabled){
 if(id.isEmpty())return false;
 QSettings().setValue("settings/packs/"+id+"/enabled",enabled);
 bool found=false;
 for(auto it=m_packs.begin();it!=m_packs.end();++it){
  auto entry=it->toMap();
  if(entry.value("id")!=id)continue;
  entry.insert(QStringLiteral("enabled"),enabled);*it=entry;found=true;
 }
 if(!found)return false;
 rebuildThemes();
 emit packsChanged();
 return true;
}
// The journal lives beside the QSettings file, so it follows the same
// per-user location (and the same isolation the test suites rely on).
QString WorkspaceBackend::sessionDirectory(){
 const QFileInfo info(QSettings().fileName());
 return info.absolutePath()+"/"+info.completeBaseName()+".session";
}
void WorkspaceBackend::scheduleSessionWrite(){if(!m_sessionTimer.isActive())m_sessionTimer.start();}
void WorkspaceBackend::flushSession(){m_sessionTimer.stop();writeSession();}
void WorkspaceBackend::writeSession(){
 if(m_sessionTimer.isActive())m_sessionTimer.stop();
 const QString directory=sessionDirectory();
 if(m_root.isEmpty()){QDir(directory).removeRecursively();return;}
 QDir().mkpath(directory+"/buffers");
 QJsonArray tabs;QSet<QString> referenced;
 for(const auto &path:orderedPaths()){
  Document &doc=m_documents[path];
  QJsonObject tab{{"path",doc.path},{"language",doc.language},{"dirty",doc.dirty},{"version",doc.version},
                  {"existed",QFileInfo::exists(doc.path)},{"diskHash",QString::fromLatin1(doc.diskHash.toHex())}};
  if(doc.dirty){
   const QString name=bufferName(doc.path);
   const bool alreadyJournaled=!doc.journaled.isNull()&&doc.journaled==doc.text;
   if(!alreadyJournaled){
    QSaveFile out(directory+"/buffers/"+name);
    const QByteArray bytes=doc.text.toUtf8();
    if(out.open(QIODevice::WriteOnly)&&out.write(bytes)==bytes.size()&&out.commit())doc.journaled=doc.text;
   }
   tab["buffer"]=name;referenced.insert(name);
  }
  tabs.append(tab);
 }
 const QDir buffers(directory+"/buffers");
 for(const auto &file:buffers.entryInfoList(QDir::Files))
  if(!referenced.contains(file.fileName()))QFile::remove(file.filePath());
 QJsonObject meta{{"version",SessionVersion},{"root",m_root},{"active",m_active},{"tabs",tabs}};
 QSaveFile out(directory+"/session.json");
 const QByteArray bytes=QJsonDocument(meta).toJson(QJsonDocument::Compact);
 if(out.open(QIODevice::WriteOnly)&&out.write(bytes)==bytes.size())out.commit();
}
// Reads the previous run's journal once, up front. Buffer files are pulled into
// memory here because opening a workspace rewrites the journal on disk.
void WorkspaceBackend::loadSession(){
 if(!m_preferences.value("files/restoreSession",true).toBool())return;
 QFile stored(sessionDirectory()+"/session.json");
 if(!stored.open(QIODevice::ReadOnly))return;
 const QJsonObject meta=QJsonDocument::fromJson(stored.readAll()).object();
 if(meta.value("version").toInt()!=SessionVersion)return;
 const QString root=meta.value("root").toString();
 if(root.isEmpty()||!QFileInfo(root).isDir())return;
 m_pendingRoot=root;
 m_pendingActive=meta.value("active").toString();
 m_pendingTabs=meta.value("tabs").toArray();
 qint64 budget=32*1024*1024;
 for(const auto &value:std::as_const(m_pendingTabs)){
  const QJsonObject tab=value.toObject();
  if(!tab.value("dirty").toBool())continue;
  const QString name=tab.value("buffer").toString();
  if(name.isEmpty())continue;
  QFile file(sessionDirectory()+"/buffers/"+name);
  if(!file.open(QIODevice::ReadOnly))continue;
  const QByteArray bytes=file.readAll();
  if(bytes.size()>budget)continue;
  budget-=bytes.size();
  m_pendingBuffers.insert(tab.value("path").toString(),QString::fromUtf8(bytes));
 }
 m_pendingSession=true;
}
bool WorkspaceBackend::restoreSession(){
 if(!m_pendingSession)return false;
 m_pendingSession=false;
 const QJsonArray tabs=m_pendingTabs;
 const QMap<QString,QString> buffers=m_pendingBuffers;
 const QString sessionRoot=m_pendingRoot,active=m_pendingActive;
 m_pendingTabs=QJsonArray();m_pendingBuffers.clear();m_pendingRoot.clear();m_pendingActive.clear();
 if(!m_preferences.value("files/restoreSession",true).toBool())return false;
 if(!m_documents.isEmpty())return false;
 if(m_root!=sessionRoot){
  // A folder opened explicitly (command line or Open Folder) wins over the journal.
  if(!m_root.isEmpty())return false;
  if(!openWorkspace(sessionRoot))return false;
 }
 QStringList changed;
 for(const auto &value:tabs){
  if(m_documents.size()>=60)break;
  const QJsonObject tab=value.toObject();
  const QString path=tab.value("path").toString();
  if(path.isEmpty()||m_documents.contains(path))continue;
  const bool dirty=tab.value("dirty").toBool(),existed=tab.value("existed").toBool();
  QFile file(path);
  const bool onDisk=file.open(QIODevice::ReadOnly);
  const QByteArray bytes=onDisk?file.readAll():QByteArray();
  QStringDecoder decoder(QStringDecoder::Utf8);
  const QString text=onDisk?QString(decoder(bytes)):QString();
  const bool readable=onDisk&&bytes.size()<=MaxFile&&!bytes.contains('\0')&&!decoder.hasError();
  if(dirty&&buffers.contains(path)){
   // The journaled disk hash stays authoritative, so a save against a file that
   // moved on while DWCode was closed is refused instead of overwriting it.
   const QByteArray hash=existed?QByteArray::fromHex(tab.value("diskHash").toString().toLatin1()):digest({});
   m_documents.insert(path,Document{path,buffers.value(path),language(path),hash,qMax(1,tab.value("version").toInt(1)),true,QString()});
   m_order.append(path);
   if(existed&&digest(bytes)!=hash)changed.append(path);
   continue;
  }
  if(!readable)continue;
  m_documents.insert(path,Document{path,text,language(path),digest(bytes),1,false,QString()});
  m_order.append(path);
 }
 m_active=m_documents.contains(active)?active:(m_order.isEmpty()?QString():m_order.first());
 watchWorkspace();emit documentStateChanged();
 if(!m_active.isEmpty())emit documentActivated(activeDocument());
 flushSession();
 for(const auto &path:changed)QTimer::singleShot(0,this,[this,path]{checkExternalChange(path);});
 const int count=m_documents.size();
 if(count>0)report(QString("Restored %1 file%2 from your last session.").arg(count).arg(count==1?"":"s"));
 return true;
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
