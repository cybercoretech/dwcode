#include "workspacebackend.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSettings>
#include <QUrl>
class BackendTest:public QObject{
 Q_OBJECT
 QTemporaryDir settings;
 static void put(const QString &path,const QByteArray &text){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(text),text.size());}
 static QByteArray get(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}
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
 void terminal(){
  QTemporaryDir root;WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));QVERIFY(b.runCommand("pwd; printf 'stdout-ok'; printf 'stderr-ok' >&2"));QTRY_VERIFY_WITH_TIMEOUT(b.terminalOutput().contains("[exit 0]"),5000);
  QVERIFY(b.terminalOutput().contains(root.path()));QVERIFY(b.terminalOutput().contains("stdout-ok"));QVERIFY(b.terminalOutput().contains("stderr-ok"));
  b.clearTerminal();QVERIFY(b.runCommand("read line; printf 'received:%s' \"$line\""));QTRY_VERIFY(b.terminalRunning());b.sendInput("hello");QTRY_VERIFY_WITH_TIMEOUT(b.terminalOutput().contains("received:hello"),5000);QTRY_VERIFY(!b.terminalRunning());
  QVERIFY(b.runCommand("sleep 60"));QTRY_VERIFY(b.terminalRunning());b.stopCommand();QVERIFY(!b.terminalRunning());
 }
 void themes(){WorkspaceBackend b;QVERIFY(b.themes().size()>=80);b.setTheme("cyberpunk-chrome");QCOMPARE(b.theme().value("id").toString(),QString("cyberpunk-chrome"));b.setTheme("tokyo-night");WorkspaceBackend restored;QCOMPARE(restored.theme().value("id").toString(),QString("tokyo-night"));}
 void languageServer(){
  QTemporaryDir root;put(root.filePath("main.ts"),"const message: string = 123;\nmessage.toUpperCase();\n");WorkspaceBackend b;QVERIFY(b.openWorkspace(root.path()));auto doc=b.openFile("main.ts");QSignalSpy diagnostics(&b,&WorkspaceBackend::diagnosticsReceived);QSignalSpy replies(&b,&WorkspaceBackend::languageReply);
  connect(&b,&WorkspaceBackend::lspChanged,&b,[&b]{qInfo() << b.lspStatus();});b.startLanguageServer();QTRY_VERIFY_WITH_TIMEOUT(b.lspReady(),20000);QTRY_VERIFY_WITH_TIMEOUT(!b.problems().isEmpty(),20000);
  QVariantMap params{{"textDocument",QVariantMap{{"uri",doc["uri"]}}},{"position",QVariantMap{{"line",1},{"character",8}}}};
  const int completion=b.languageRequest("textDocument/completion",params);QVERIFY(completion>0);QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);auto reply=replies.takeFirst();QCOMPARE(reply[0].toInt(),completion);QVERIFY(reply[2].toString().isEmpty());const auto items=reply[1].toMap().value("items").toList();QVERIFY(!items.isEmpty());
  params["position"]=QVariantMap{{"line",0},{"character",7}};const int hover=b.languageRequest("textDocument/hover",params);QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty(),10000);reply=replies.takeFirst();QCOMPARE(reply[0].toInt(),hover);QVERIFY(!reply[1].toMap().isEmpty());
  QVERIFY(b.updateDocument(doc["path"].toString(),"const message: string = 'hello';\nmessage.toUpperCase();\n"));QTRY_VERIFY_WITH_TIMEOUT(b.problems().isEmpty(),20000);b.stopLanguageServer();QVERIFY(!b.lspReady());
 }
};
QTEST_GUILESS_MAIN(BackendTest)
#include "backend_test.moc"
