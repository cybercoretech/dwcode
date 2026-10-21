// Drives the real Main.qml window offscreen: clicks, key presses and Monaco
// actions go through the same paths a user's input does.
#include "localwebserver.h"
#include "workspacebackend.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QSettings>
#include <QWheelEvent>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include <memory>

class Probe:public QObject{
 Q_OBJECT
public:
 QMap<int,QString> results;
 Q_INVOKABLE void put(int id,const QString &value){results.insert(id,value);}
};

struct App{
 std::unique_ptr<WorkspaceBackend> backend;
 std::unique_ptr<QQmlApplicationEngine> engine;
 QQuickWindow *window=nullptr;
};

class UiTest:public QObject{
 Q_OBJECT
 QTemporaryDir settings,root;
 LocalWebServer server{QStringLiteral(DWCODE_WEB_ROOT)};
 App app;
 Probe probe;
 int nextId=0;
 const QString source="const message: string = 123;\nmessage.toUpperCase();\nexport function shout(text: string) { return text; }\nshout(message);\n";

 static void put(const QString &path,const QByteArray &text){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(text),text.size());}
 static QByteArray get(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}

 void launch(App &a){
  a.backend=std::make_unique<WorkspaceBackend>();
  QVERIFY(a.backend->openWorkspace(root.path()));
  a.engine=std::make_unique<QQmlApplicationEngine>();
  a.engine->rootContext()->setContextProperty("workspaceBackend",a.backend.get());
  a.engine->rootContext()->setContextProperty("webEditorUrl",server.url());
  a.engine->globalObject().setProperty("probe",a.engine->newQObject(&probe));
  QQmlEngine::setObjectOwnership(&probe,QQmlEngine::CppOwnership);
  a.engine->load(QUrl("qrc:/qml/Main.qml"));
  QVERIFY(!a.engine->rootObjects().isEmpty());
  a.window=qobject_cast<QQuickWindow*>(a.engine->rootObjects().first());
  QVERIFY(a.window);
  QVERIFY(QTest::qWaitForWindowExposed(a.window));
 }
 void shutdown(App &a){a.engine.reset();a.backend.reset();a.window=nullptr;QTest::qWait(200);}
 QObject *view(const char *name){return app.window->findChild<QObject*>(name);}
 QObject *editor(){return view("editorView");}
 QObject *split(){
  for(auto *v:app.window->findChildren<QObject*>())
   if(v->inherits("QQuickWebEngineView")&&v->objectName().isEmpty())return v;
  return nullptr;
 }
 // Evaluates an expression in a WebEngineView and returns its JSON encoding.
 QString js(QObject *target,const QString &expr){
  const int id=++nextId;
  QJSValue cb=app.engine->evaluate("(function(id){return function(r){probe.put(id,r===undefined?'undefined':r)}})").call({id});
  const QString script="(()=>{try{return JSON.stringify((()=>{return ("+expr+")})())}catch(e){return JSON.stringify('ERR '+e)}})()";
  QMetaObject::invokeMethod(target,"runJavaScript",Q_ARG(QString,script),Q_ARG(QJSValue,cb));
  if(!QTest::qWaitFor([&]{return probe.results.contains(id);},10000))return "TIMEOUT";
  return probe.results.take(id);
 }
 bool waitJs(QObject *target,const QString &expr,int ms=20000){return QTest::qWaitFor([&]{return js(target,expr)=="true";},ms);}
 QList<QQuickItem*> items(){
  QList<QQuickItem*> out;std::function<void(QQuickItem*)> walk=[&](QQuickItem *i){out.append(i);for(auto *c:i->childItems())walk(c);};
  walk(app.window->contentItem());
  for(auto *popup:app.window->findChildren<QObject*>())if(popup->inherits("QQuickPopup")){auto *c=popup->property("contentItem").value<QQuickItem*>();if(c)walk(c);}
  return out;
 }
 QQuickItem *withText(const QString &text,const char *type=nullptr){
  for(auto *item:items())
   if(item->isVisible()&&item->property("text").toString()==text&&(!type||item->inherits(type)))return item;
  return nullptr;
 }
 void click(QQuickItem *item){
  QVERIFY2(item,"missing item");
  QTest::mouseClick(app.window,Qt::LeftButton,{},item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());
 }
 QQuickItem *combo(){
  for(auto *item:items())if(item->inherits("QQuickComboBox"))return item;
  return nullptr;
 }
 QString editorBackground(QObject *target){return js(target,"getComputedStyle(document.querySelector('.monaco-editor .monaco-editor-background')).backgroundColor");}
 void typeText(const QString &text){for(const QChar c:text)QTest::keyClick(app.window,c.toLatin1());}
 static QString rgb(const QString &hex){const QColor c(hex);return QString("\"rgb(%1, %2, %3)\"").arg(c.red()).arg(c.green()).arg(c.blue());}
 QVariantMap themeById(const QString &id){for(const auto &t:app.backend->themes())if(t.toMap()["id"]==id)return t.toMap();return {};}

private slots:
 void initTestCase(){
  QCoreApplication::setOrganizationName("DWCodeTests");QCoreApplication::setApplicationName("UiIsolated");
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  QVERIFY(server.start());
  put(root.filePath("main.ts"),source.toUtf8());put(root.filePath("notes.md"),"# notes\n");
  launch(app);
  QVERIFY2(waitJs(editor(),"window.dwcodeNativeReady===true && !!window.dwcodeEditor.getModel()"),"editor never connected to native backend");
  QCOMPARE(js(editor(),"window.dwcodeEditor.getModel().uri.path.endsWith('/main.ts')"),QString("true"));
 }
 void themeDropdownOpensScrollsAndApplies(){
  auto *picker=combo();QVERIFY(picker);
  QCOMPARE(picker->property("count").toInt(),app.backend->themes().size());
  QVERIFY(app.backend->themes().size()>=84);
  click(picker);
  auto *popup=picker->property("popup").value<QObject*>();QVERIFY(popup);
  QTRY_VERIFY(popup->property("opened").toBool());
  auto *list=popup->property("contentItem").value<QQuickItem*>();QVERIFY(list);
  QTRY_VERIFY(list->property("height").toReal()>100);
  const qreal top=list->property("contentY").toReal();
  const QPointF at=list->mapToScene(QPointF(list->width()/2,list->height()/2));
  QWheelEvent wheel(at,app.window->mapToGlobal(at),QPoint(),QPoint(0,-120*6),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
  QCoreApplication::sendEvent(app.window,&wheel);
  QTRY_VERIFY2(list->property("contentY").toReal()>top+50,"wheel did not scroll the theme list");
  QTest::keyClick(app.window,Qt::Key_Escape);
  QTRY_VERIFY(!popup->property("opened").toBool());

  // Open again and choose a theme far down the list with the keyboard.
  click(picker);QTRY_VERIFY(popup->property("opened").toBool());
  const int from=picker->property("highlightedIndex").toInt(),target=60;
  for(int i=from;i<target;++i)QTest::keyClick(app.window,Qt::Key_Down);
  QTRY_COMPARE(picker->property("highlightedIndex").toInt(),target);
  QVERIFY(list->property("contentY").toReal()>top+500);
  QTest::keyClick(app.window,Qt::Key_Return);
  QTRY_VERIFY(!popup->property("opened").toBool());
  const auto chosen=app.backend->themes().at(target).toMap();
  QCOMPARE(app.backend->theme()["id"],chosen["id"]);
  QCOMPARE(picker->property("currentIndex").toInt(),target);
  QCOMPARE(QColor(app.window->property("color").value<QColor>()),QColor(chosen["surfaces"].toMap()["window"].toString()));
  QTRY_COMPARE(editorBackground(editor()),rgb(chosen["surfaces"].toMap()["editor"].toString()));
  qInfo()<<"selected"<<chosen["name"].toString()<<"via dropdown";
 }
 void splitViewSharesThemeAndText(){
  click(withText("Split view"));
  QTRY_VERIFY(app.window->property("splitVisible").toBool());
  QObject *second=split();QVERIFY(second);
  QVERIFY2(waitJs(second,"window.dwcodeNativeReady===true && !!window.dwcodeEditor.getModel()"),"split editor never connected");
  QTRY_COMPARE(editorBackground(second),rgb(app.backend->theme()["surfaces"].toMap()["editor"].toString()));
  // Typing in one view reaches the other through the shared native document.
  QCOMPARE(js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:1,column:1}),window.dwcodeEditor.trigger('keyboard','type',{text:'// split check\\n'}),true)"),QString("true"));
  QVERIFY(waitJs(second,"window.dwcodeEditor.getValue().startsWith('// split check')"));
  QVERIFY(app.backend->dirty());
  QVERIFY(withText("● main.ts"));
  // Switching theme with the split open recolours both views.
  app.backend->setTheme("cyberpunk-chrome");
  const QString want=rgb(themeById("cyberpunk-chrome")["surfaces"].toMap()["editor"].toString());
  QTRY_COMPARE(editorBackground(editor()),want);QTRY_COMPARE(editorBackground(second),want);
  click(withText("Single view"));QTRY_VERIFY(!app.window->property("splitVisible").toBool());
 }
 void saveFromEditorKeyboardAndButton(){
  const QString path=root.filePath("main.ts");
  QVERIFY(get(path).startsWith("const message"));
  // Ctrl+S with focus inside Monaco.
  QMetaObject::invokeMethod(editor(),"forceActiveFocus");js(editor(),"(window.dwcodeEditor.focus(),true)");
  QTest::keyClick(app.window,Qt::Key_S,Qt::ControlModifier);
  QTRY_VERIFY_WITH_TIMEOUT(get(path).startsWith("// split check\n"),5000);
  QTRY_VERIFY(!app.backend->dirty());
  QVERIFY(withText("main.ts"));
  // Edit again, then use the toolbar Save button.
  js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:1,column:1}),window.dwcodeEditor.trigger('keyboard','type',{text:'// button\\n'}),true)");
  QTRY_VERIFY(app.backend->dirty());
  click(withText("Save ●"));
  QTRY_VERIFY_WITH_TIMEOUT(get(path).startsWith("// button\n// split check\n"),5000);
  // A change made on disk behind the editor's back blocks the save.
  js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:1,column:1}),window.dwcodeEditor.trigger('keyboard','type',{text:'// mine\\n'}),true)");
  QTRY_VERIFY(app.backend->dirty());
  put(path,"// theirs\n");
  click(withText("Save ●"));
  QTRY_VERIFY(app.backend->message().startsWith("Save blocked"));
  QCOMPARE(get(path),QByteArray("// theirs\n"));
  // Restore a clean, known file for the language-server test.
  app.backend->discardChanges();put(path,source.toUtf8());app.backend->openFile("main.ts");
  QVERIFY(waitJs(editor(),"window.dwcodeEditor.getValue().startsWith('const message')"));
 }
 void terminalRunsSendsInputAndStops(){
  click(withText("TERMINAL"));
  QTRY_COMPARE(app.window->property("activeBottomTab").toString(),QString("TERMINAL"));
  QQuickItem *field=nullptr;
  for(auto *i:items())if(i->inherits("QQuickTextField")&&i->isVisible()&&i->property("placeholderText").toString().startsWith("Run a command"))field=i;
  QVERIFY(field);
  click(field);typeText("echo ui-$((6*7)) && pwd");QTest::keyClick(app.window,Qt::Key_Return);
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->terminalOutput().contains("ui-42"),5000);
  QTRY_VERIFY(app.backend->terminalOutput().contains("[exit 0]"));
  QVERIFY(app.backend->terminalOutput().contains(root.path()));
  click(field);typeText("read line; echo got:$line");click(withText("Run"));
  QTRY_VERIFY(app.backend->terminalRunning());
  QTRY_VERIFY(withText("Send"));
  click(field);typeText("cyber");QTest::keyClick(app.window,Qt::Key_Return);
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->terminalOutput().contains("got:cyber"),5000);
  QTRY_VERIFY(!app.backend->terminalRunning());
  click(field);typeText("sleep 60");QTest::keyClick(app.window,Qt::Key_Return);
  QTRY_VERIFY(app.backend->terminalRunning());
  auto *stop=withText("Stop");QVERIFY(stop);QTRY_VERIFY(stop->property("enabled").toBool());click(stop);
  QTRY_VERIFY_WITH_TIMEOUT(!app.backend->terminalRunning(),3000);
  click(withText("Clear"));QTRY_VERIFY(app.backend->terminalOutput().isEmpty());
 }
 void languageServerThroughEditor(){
  click(withText("Connect LSP"));
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->lspReady(),20000);
  QVERIFY(withText("LSP connected"));
  QTRY_VERIFY_WITH_TIMEOUT(!app.backend->problems().isEmpty(),20000);
  QTRY_VERIFY(withText(QString("PROBLEMS (%1)").arg(app.backend->problems().size())));
  QVERIFY2(waitJs(editor(),"document.querySelectorAll('.squiggly-error').length>0"),"no error squiggle drawn in Monaco");
  // Hover over `message` on line 1.
  js(editor(),"(window.dwcodeEditor.focus(),window.dwcodeEditor.setPosition({lineNumber:1,column:8}),window.dwcodeEditor.trigger('test','editor.action.showHover',{}),true)");
  QVERIFY2(waitJs(editor(),"[...document.querySelectorAll('.monaco-hover-content')].some(n=>n.textContent.includes('const message: string'))"),"hover text missing");
  // Completion after `message.` on line 2. The list is virtualised, so check string-only
  // members near the top; Monaco's word-based fallback cannot produce these.
  js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:2,column:9}),window.dwcodeEditor.trigger('test','editor.action.triggerSuggest',{}),true)");
  QVERIFY2(waitJs(editor(),"[...document.querySelectorAll('.suggest-widget .monaco-list-row')].some(n=>n.textContent.includes('charCodeAt'))"),"completion list missing string members");
  js(editor(),"(window.dwcodeEditor.trigger('test','hideSuggestWidget',{}),true)");
  // Go to definition of `shout` on line 4 jumps to line 3.
  js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:4,column:2}),window.dwcodeEditor.trigger('test','editor.action.revealDefinition',{}),true)");
  QVERIFY2(waitJs(editor(),"window.dwcodeEditor.getPosition().lineNumber===3"),"definition did not move the cursor");
  // Fixing the error in the editor clears the problem list.
  js(editor(),"(window.dwcodeEditor.executeEdits('test',[{range:{startLineNumber:1,startColumn:25,endLineNumber:1,endColumn:28},text:\"'ok'\"}]),true)");
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->problems().isEmpty(),20000);
  QVERIFY(waitJs(editor(),"document.querySelectorAll('.squiggly-error').length===0"));
  app.backend->stopLanguageServer();QVERIFY(!app.backend->lspReady());
  app.backend->discardChanges();
 }
 void themeSurvivesRestart(){
  app.backend->setTheme("tokyo-night");
  shutdown(app);
  launch(app);
  QCOMPARE(app.backend->theme()["id"].toString(),QString("tokyo-night"));
  QCOMPARE(combo()->property("currentText").toString(),themeById("tokyo-night")["name"].toString());
  QVERIFY(waitJs(editor(),"window.dwcodeNativeReady===true"));
  QTRY_COMPARE(editorBackground(editor()),rgb(themeById("tokyo-night")["surfaces"].toMap()["editor"].toString()));
 }
 void cleanupTestCase(){shutdown(app);}
};

int main(int argc,char **argv){
 // Never open a window on the developer's desktop; DWCODE_UI_PLATFORM overrides.
 qputenv("QT_QPA_PLATFORM",qEnvironmentVariableIsEmpty("DWCODE_UI_PLATFORM")?"offscreen":qgetenv("DWCODE_UI_PLATFORM"));
 qunsetenv("WAYLAND_DISPLAY");
 qputenv("QT_QPA_PLATFORMTHEME","");
 if(qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))qputenv("QT_QUICK_BACKEND","software");
 if(qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS"))qputenv("QTWEBENGINE_CHROMIUM_FLAGS","--disable-gpu");
 QtWebEngineQuick::initialize();
 QGuiApplication a(argc,argv);
 UiTest t;
 return QTest::qExec(&t,argc,argv);
}
#include "ui_test.moc"
