// Drives the real Main.qml window offscreen: clicks, key presses and Monaco
// actions go through the same paths a user's input does.
#include "ui_harness.h"

class UiTest:public UiHarness{
 Q_OBJECT
 const QString source="const message: string = 123;\nmessage.toUpperCase();\nexport function shout(text: string) { return text; }\nshout(message);\n";


private slots:
 void initTestCase(){
  boot("UiIsolated");
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
  QTRY_VERIFY(list->property("contentY").toReal()>top+500); // scrolling is animated
  QTest::keyClick(app.window,Qt::Key_Return);
  QTRY_VERIFY(!popup->property("opened").toBool());
  const auto chosen=app.backend->themes().at(target).toMap();
  QCOMPARE(app.backend->theme()["id"],chosen["id"]);
  QCOMPARE(picker->property("currentIndex").toInt(),target);
  QCOMPARE(QColor(app.window->property("color").value<QColor>()),QColor(chosen["surfaces"].toMap()["window"].toString()));
  QTRY_COMPARE(editorBackground(editor()),rgb(chosen["surfaces"].toMap()["editor"].toString()));
  qInfo()<<"selected"<<chosen["name"].toString()<<"via dropdown";
 }
 void everyThemeReachesTheEditor(){
  for(const auto &value:app.backend->themes()){
   const auto t=value.toMap();app.backend->setTheme(t["id"].toString());
   QTRY_COMPARE_WITH_TIMEOUT(editorBackground(editor()),rgb(t["surfaces"].toMap()["editor"].toString()),3000);
  }
 }
 void splitViewSharesThemeAndText(){
  click(withText("Split editor"));
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
  click(field);typeText("read line; echo got:$line");click(withText("Run","QQuickButton"));
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
  // Real keystrokes: the backend's echo of each change must not rewind the view,
  // which would cancel the completion list opened by the '.'.
  js(editor(),"(window.dwcodeEditor.setPosition({lineNumber:5,column:1}),true)");
  QMetaObject::invokeMethod(editor(),"forceActiveFocus");typeText("message.");
  QVERIFY2(waitJs(editor(),"[...document.querySelectorAll('.suggest-widget .monaco-list-row')].some(n=>n.textContent.includes('charAt'))"),"typing 'message.' did not show its members");
  QCOMPARE(js(editor(),"window.dwcodeEditor.getModel().getLineContent(5)"),QString("\"message.\""));
  js(editor(),"(window.dwcodeEditor.trigger('test','hideSuggestWidget',{}),window.dwcodeEditor.trigger('test','undo',{}),true)");
  QVERIFY(waitJs(editor(),"window.dwcodeEditor.getModel().getLineContent(5)===''"));
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
 prepareUiEnvironment();
 QtWebEngineQuick::initialize();
 QGuiApplication a(argc,argv);
 UiTest t;
 return QTest::qExec(&t,argc,argv);
}
#include "ui_test.moc"
