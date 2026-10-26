#include "workspacebackend.h"
#include "packloader.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QUrl>
class BackendTest:public QObject{
 Q_OBJECT
 QTemporaryDir settings;
 static void put(const QString &path,const QByteArray &text){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(text),text.size());}
 static QByteArray get(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}
 // Pack fixtures live as real files: JSON stays validatable by the same tools
 // CI runs, and the test only has to drop them where DWCode looks.
 static bool copyTree(const QString &from,const QString &to){
  const QFileInfo source(from);
  if(source.isFile()){
   QDir().mkpath(QFileInfo(to).absolutePath());
   if(QFile::exists(to))QFile::remove(to);
   return QFile::copy(from,to);
  }
  QDir().mkpath(to);const QDir dir(from);
  if(!dir.exists())return false;
  for(const auto &entry:dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
   if(!copyTree(from+QLatin1Char('/')+entry,to+QLatin1Char('/')+entry))return false;
  for(const auto &entry:dir.entryList(QDir::Files))
   if(!copyTree(from+QLatin1Char('/')+entry,to+QLatin1Char('/')+entry))return false;
  return true;
 }
 QString fixturePacks(){const QString found=QFINDTESTDATA("fixtures/packs");return found;}
private slots:
 void initTestCase(){QCoreApplication::setOrganizationName("DWCodeTests");QCoreApplication::setApplicationName("Isolated");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());}
 void files(){
  QTemporaryDir root,other;put(root.filePath("main.ts"),"const x = 1;\n");put(other.filePath("outside.txt"),"private");
  WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));auto doc=b.openFile("main.ts");QVERIFY(!doc.isEmpty());const auto path=doc["path"].toString();
  QVERIFY(b.updateDocument(path,"const x = 2;\n"));QVERIFY(b.dirty());QVERIFY(!b.openWorkspace(other.path()));QVERIFY(b.saveDocument(path));QVERIFY(!b.dirty());QCOMPARE(get(path),QByteArray("const x = 2;\n"));
  QVERIFY(b.updateDocument(path,"const x = 3;\n"));put(path,"external edit\n");QVERIFY(!b.saveDocument(path));QCOMPARE(get(path),QByteArray("external edit\n"));QVERIFY(b.dirty());
  QVERIFY(b.saveAs(root.filePath("copy.ts")));QCOMPARE(get(root.filePath("copy.ts")),QByteArray("const x = 3;\n"));QVERIFY(!b.saveAs(other.filePath("leak.ts")));
  QVERIFY(b.openFile(other.filePath("outside.txt")).isEmpty());QVERIFY(QFile::link(other.filePath("outside.txt"),root.filePath("escape.txt")));QVERIFY(b.openFile("escape.txt").isEmpty());
  put(root.filePath("binary"),QByteArray("a\0b",3));QVERIFY(b.openFile("binary").isEmpty());
 }
 void newFilesAndExternalChanges(){
  QTemporaryDir root;put(root.filePath("main.ts"),"const local = 1;\n");WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));
  QVERIFY(b.createFile("src/missing.ts")==false);QVERIFY(b.createFile("new.ts"));const auto created=b.activeDocument();QVERIFY(created.value("dirty").toBool());QCOMPARE(created.value("name").toString(),QString("new.ts"));
  QVERIFY(b.saveDocument(created.value("path").toString()));QCOMPARE(get(root.filePath("new.ts")),QByteArray());QVERIFY(!b.createFile("new.ts"));QVERIFY(!b.createFile("../outside.ts"));
  const auto original=b.openFile("main.ts");QVERIFY(!original.isEmpty());const auto path=original.value("path").toString();QSignalSpy changes(&b,&WorkspaceBackend::documentExternalChange);
  put(path,"const disk = 2;\n");QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(),5000);auto change=changes.takeFirst().at(0).toMap();QVERIFY(!change.value("dirty").toBool());QVERIFY(b.reloadDocument(path));QCOMPARE(b.activeDocument().value("text").toString(),QString("const disk = 2;\n"));
  QVERIFY(b.updateDocument(path,"const mine = 3;\n"));put(path,"const theirs = 4;\n");QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(),5000);change=changes.takeFirst().at(0).toMap();QVERIFY(change.value("dirty").toBool());QCOMPARE(b.activeDocument().value("text").toString(),QString("const mine = 3;\n"));
  put(path,"const theirsAgain = 5;\n");QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(),5000);QVERIFY(changes.takeFirst().at(0).toMap().value("dirty").toBool());
 }
 void terminal(){
  QTemporaryDir root;WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));QVERIFY(b.runCommand("pwd; printf 'stdout-ok'; printf 'stderr-ok' >&2"));QTRY_VERIFY_WITH_TIMEOUT(b.terminalOutput().contains("[exit 0]"),5000);
  QVERIFY(b.terminalOutput().contains(root.path()));QVERIFY(b.terminalOutput().contains("stdout-ok"));QVERIFY(b.terminalOutput().contains("stderr-ok"));
  b.clearTerminal();QVERIFY(b.runCommand("read line; printf 'received:%s' \"$line\""));QTRY_VERIFY(b.terminalRunning());b.sendInput("hello");QTRY_VERIFY_WITH_TIMEOUT(b.terminalOutput().contains("received:hello"),5000);QTRY_VERIFY(!b.terminalRunning());
  QVERIFY(b.runCommand("sleep 60"));QTRY_VERIFY(b.terminalRunning());b.stopCommand();QVERIFY(!b.terminalRunning());
 }
 void preferencesPersistAndAffectFiles(){
  QTemporaryDir root;put(root.filePath("visible.ts"),"export {}\n");put(root.filePath("ignored.log"),"ignore\n");
  WorkspaceBackend first;first.setPreference("editor/fontSize",19);first.setPreference("files/excludes","*.log");
  QVERIFY(first.openWorkspace(root.path()));
  for(const auto &entry:first.files())QVERIFY(entry.toMap().value("name")!="ignored.log");
  WorkspaceBackend second;QCOMPARE(second.preferences().value("editor/fontSize").toInt(),19);
  second.resetPreferences();QCOMPARE(second.preferences().value("editor/fontSize").toInt(),14);
 }
 void themes(){WorkspaceBackend b;QVERIFY(b.themes().size()>=80);b.setTheme("cyberpunk-chrome");QCOMPARE(b.theme().value("id").toString(),QString("cyberpunk-chrome"));b.setTheme("tokyo-night");WorkspaceBackend restored;QCOMPARE(restored.theme().value("id").toString(),QString("tokyo-night"));}
 void packLoader(){
  const QString packs=WorkspaceBackend::packDirectory();
  const QString unit=packs+"/unit-pack";
  const QString fixtures=fixturePacks();
  QVERIFY2(!fixtures.isEmpty(),"tests/fixtures/packs is missing");
  QVERIFY(copyTree(fixtures+"/unit-pack",unit));
  {
   WorkspaceBackend b;
   QVariantMap pack;for(const auto &value:b.packs())if(value.toMap().value("id")=="unit.pack")pack=value.toMap();
   QVERIFY(!pack.isEmpty());
   QVERIFY2(pack.value("valid").toBool(),qPrintable(pack.value("errorText").toString()));
   QVERIFY(pack.value("enabled").toBool());
   QCOMPARE(pack.value("themes").toList().size(),1);
   QCOMPARE(pack.value("license").toString(),QString("MIT"));
   QVERIFY(pack.value("note").toString().contains("snippets: javascript"));
   // The pack's data reaches both catalogs the app serves.
   QVERIFY(b.packSnippets().contains("javascript"));
   const int javascript=b.packSnippets().value("javascript").toList().size();
   QCOMPARE(javascript,3); // bundled pack contributes two, unit pack one
   bool merged=false;
   for(const auto &value:b.themes())if(value.toMap().value("id")=="unit.test-theme")merged=true;
   QVERIFY(merged);
   const int before=b.themes().size();
   QVERIFY(b.setPackEnabled("unit.pack",false));
   QCOMPARE(b.themes().size(),before-1);
   QCOMPARE(b.packSnippets().value("javascript").toList().size(),javascript-1);
  }
  { // the switch persists across instances
   WorkspaceBackend b;
   QVariantMap pack;for(const auto &value:b.packs())if(value.toMap().value("id")=="unit.pack")pack=value.toMap();
   QVERIFY(!pack.value("enabled").toBool());
   QVERIFY(pack.value("valid").toBool());
   QVERIFY(b.setPackEnabled("unit.pack",true));
  }
  // Rejected manifests stay listed with a reason and contribute nothing.
  QVERIFY(copyTree(fixtures+"/bad-pack",packs+"/bad-pack"));
  // A resource pointing outside the pack is refused even when it exists.
  QVERIFY(copyTree(fixtures+"/outside.json",packs+"/outside.json"));
  QVERIFY(copyTree(fixtures+"/escape-pack",packs+"/escape-pack"));
  {
   WorkspaceBackend b;
   QVariantMap found;
   for(const auto &value:b.packs()){
    const auto pack=value.toMap();
    if(pack.value("id")=="unit.bad")found=pack;
   }
   QVERIFY(!found.isEmpty());
   QVERIFY(!found.value("valid").toBool());
   QVERIFY(found.value("errorText").toString().contains("manifestVersion must be 1"));
   QVERIFY(found.value("errorText").toString().contains("executable entry points"));
   QVERIFY(found.value("errorText").toString().contains("cannot request capabilities"));
   QVERIFY(found.value("errorText").toString().contains("not compatible with DWCode"));
   QVariantMap escaped;
   for(const auto &value:b.packs()){
    const auto pack=value.toMap();
    if(pack.value("id")=="unit.escape")escaped=pack;
   }
   QVERIFY(!escaped.isEmpty());
   QVERIFY(!escaped.value("valid").toBool());
   QVERIFY2(escaped.value("errorText").toString().contains("escapes the pack"),qPrintable(escaped.value("errorText").toString()));
   QVERIFY(!b.packSnippets().contains("escape"));
  }
  // Compatibility windows are checked per manifest, fail-closed.
  QVERIFY(PackLoader::versionSatisfies(">=0.1.0 <1.0.0","0.1.0"));
  QVERIFY(!PackLoader::versionSatisfies(">=0.2.0","0.1.0"));
  QVERIFY(PackLoader::versionSatisfies(">=0.1.0","0.1.0"));
  QVERIFY(!PackLoader::versionSatisfies(">0.1.0","0.1.0"));
  QVERIFY(PackLoader::versionSatisfies("<=0.1.0","0.1.0"));
  QVERIFY(!PackLoader::versionSatisfies(">=1.0.0","0.1.0"));
  QVERIFY(!PackLoader::versionSatisfies("^0.1.0","0.9.0"));
  QVERIFY(PackLoader::versionSatisfies("~1.2.3","1.2.9"));
  QVERIFY(PackLoader::versionSatisfies(">=0.1.0","0.4.2"));
  QVERIFY(!PackLoader::versionSatisfies("","0.1.0"));
  QVERIFY(!PackLoader::versionSatisfies(">=0.1.0","nonsense"));
 }
 void languageServer(){
  QTemporaryDir root;put(root.filePath("main.ts"),"const message: string = 123;\nmessage.toUpperCase();\n");WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));auto doc=b.openFile("main.ts");QSignalSpy diagnostics(&b,&WorkspaceBackend::diagnosticsReceived);QSignalSpy replies(&b,&WorkspaceBackend::languageReply);
  connect(&b,&WorkspaceBackend::lspChanged,&b,[&b]{qInfo() << b.lspStatus();});b.startLanguageServer();QTRY_VERIFY_WITH_TIMEOUT(b.lspReady(),20000);QTRY_VERIFY_WITH_TIMEOUT(!b.problems().isEmpty(),20000);
  QVariantMap params{{"textDocument",QVariantMap{{"uri",doc["uri"]}}},{"position",QVariantMap{{"line",1},{"character",8}}}};
  const int completion=b.languageRequest("textDocument/completion",params);QVERIFY(completion>0);QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);auto reply=replies.takeFirst();QCOMPARE(reply[0].toInt(),completion);QVERIFY(reply[2].toString().isEmpty());const auto items=reply[1].toMap().value("items").toList();QVERIFY(!items.isEmpty());
  params["position"]=QVariantMap{{"line",0},{"character",7}};const int hover=b.languageRequest("textDocument/hover",params);QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);reply=replies.takeFirst();QCOMPARE(reply[0].toInt(),hover);QVERIFY(!reply[1].toMap().isEmpty());
  QVERIFY(b.updateDocument(doc["path"].toString(),"const message: string = 'hello';\nmessage.toUpperCase();\n"));QTRY_VERIFY_WITH_TIMEOUT(b.problems().isEmpty(),20000);b.stopLanguageServer();QVERIFY(!b.lspReady());
 }
  void sessionRestore(){
   QTemporaryDir root;put(root.filePath("alpha.ts"),"const alpha = 1;\n");put(root.filePath("beta.ts"),"const beta = 2;\n");
   const QString alpha=QFileInfo(root.filePath("alpha.ts")).canonicalFilePath();
   const QString beta=QFileInfo(root.filePath("beta.ts")).canonicalFilePath();
   {
    WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));
    QVERIFY(!b.openFile("alpha.ts").isEmpty());QVERIFY(!b.openFile("beta.ts").isEmpty());
    QVERIFY(b.updateDocument(alpha,"const alpha = 99;\n"));
   }
   { // tab order, the active file and the unsaved buffer all come back
    WorkspaceBackend b;QVERIFY(b.restoreSession());
    QCOMPARE(b.documents().size(),2);
    QCOMPARE(b.documents().at(0).toMap().value("name").toString(),QString("alpha.ts"));
    QCOMPARE(b.documents().at(1).toMap().value("name").toString(),QString("beta.ts"));
    QCOMPARE(b.activePath(),beta);
    QVERIFY(!b.dirty());
    const auto restored=b.documents().at(0).toMap();
    QVERIFY(restored.value("dirty").toBool());
    QCOMPARE(restored.value("text").toString(),QString("const alpha = 99;\n"));
   }
   put(alpha,"const alpha = 42;\n"); // the file moved on while DWCode was closed
   {
    WorkspaceBackend b;QVERIFY(b.restoreSession());
    QSignalSpy changes(&b,&WorkspaceBackend::documentExternalChange);
    QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(),3000);
    QVERIFY(changes.takeFirst().at(0).toMap().value("dirty").toBool());
    QCOMPARE(b.documents().at(0).toMap().value("text").toString(),QString("const alpha = 99;\n"));
    QVERIFY(!b.saveDocument(alpha)); // never overwrites a version it did not read
    QCOMPARE(get(alpha),QByteArray("const alpha = 42;\n"));
   }
   QVERIFY(QFile::remove(alpha));
   { // a file deleted outside DWCode keeps its unsaved buffer
    WorkspaceBackend b;QVERIFY(b.restoreSession());
    QCOMPARE(b.documents().size(),2);
    const auto restored=b.documents().at(0).toMap();
    QVERIFY(restored.value("dirty").toBool());
    QCOMPARE(restored.value("text").toString(),QString("const alpha = 99;\n"));
   }
   {
    WorkspaceBackend b;QVERIFY(b.restoreSession());QVERIFY(b.hasUnsaved());b.discardChanges();
   }
   { // discarded work stays discarded, but the folder still reopens
    WorkspaceBackend b;QVERIFY(b.restoreSession());
    QVERIFY(!b.hasUnsaved());QCOMPARE(b.documents().size(),0);
    QCOMPARE(b.root(),QFileInfo(root.path()).canonicalFilePath());
   }
  }
};
QTEST_GUILESS_MAIN(BackendTest)
#include "backend_test.moc"
