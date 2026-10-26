// Captures real DWCode window states for the website. Usage:
//   ./site-shots <output-dir>   (defaults to build/site-shots)
// Each state is reached through the UI harness, so the images show the app
// as it behaves, not a mock-up.
#include "ui_harness.h"

class Shots:public UiHarness{
 Q_OBJECT
 QString out;
 void save(const QString &name){
  QTest::qWait(700);
  const QImage image=app.window->grabWindow();
  QVERIFY2(!image.isNull()&&image.save(out+"/"+name+".png"),qPrintable("could not save "+name));
  qInfo()<<"saved"<<name<<image.size();
 }
 void theme(const QString &id){
  app.backend->setTheme(id);
  QTRY_COMPARE(editorBackground(editor()),rgb(themeById(id)["surfaces"].toMap()["editor"].toString()));
 }
 void cursor(int line,int column){js(editor(),QString("(window.dwcodeEditor.focus(),window.dwcodeEditor.setPosition({lineNumber:%1,column:%2}),window.dwcodeEditor.revealLineInCenterIfOutsideViewport(%1),true)").arg(line).arg(column));}
 void action(const QString &id){js(editor(),"(window.dwcodeEditor.trigger('shots','"+id+"',{}),true)");}

private slots:
 void initTestCase(){
  boot("Shots");
  out=QCoreApplication::arguments().value(1,QStringLiteral(DWCODE_SOURCE_ROOT)+"/build/site-shots");
  QDir().mkpath(out);
  // A named project folder so the explorer and status bar read naturally.
  workspace=root.filePath("signal-relay");QDir(workspace).mkpath("src");
  put(QDir(workspace).filePath("README.md"),"# signal-relay\n\nRoutes tagged signals between Cybercore nodes.\n");
  put(QDir(workspace).filePath("package.json"),"{\n  \"name\": \"signal-relay\",\n  \"type\": \"module\"\n}\n");
  put(QDir(workspace).filePath("tsconfig.json"),"{\n  \"compilerOptions\": { \"strict\": true, \"target\": \"ES2022\", \"module\": \"ESNext\" }\n}\n");
  put(QDir(workspace).filePath("src/signal.ts"),
   "/** A tagged reading from one Cybercore node. */\n"
   "export type Signal = {\n  node: string;\n  channel: \"cyan\" | \"magenta\";\n  strength: number;\n  seenAt: Date;\n};\n\n"
   "export const isStrong = (signal: Signal) => signal.strength >= 0.75;\n");
  put(QDir(workspace).filePath("src/relay.ts"),
   "import { type Signal, isStrong } from \"./signal\";\n\n"
   "/** Groups strong signals by channel, newest first. */\n"
   "export function route(signals: Signal[]): Map<Signal[\"channel\"], Signal[]> {\n"
   "  const routes = new Map<Signal[\"channel\"], Signal[]>();\n"
   "  for (const signal of signals.filter(isStrong)) {\n"
   "    const lane = routes.get(signal.channel) ?? [];\n"
   "    lane.push(signal);\n"
   "    routes.set(signal.channel, lane);\n"
   "  }\n"
   "  for (const lane of routes.values()) lane.sort((a, b) => +b.seenAt - +a.seenAt);\n"
   "  return routes;\n"
   "}\n\n"
   "const routes = route([\n"
   "  { node: \"darkbox\", channel: \"cyan\", strength: 0.92, seenAt: new Date() },\n"
   "  { node: \"blackbox\", channel: \"magenta\", strength: \"0.81\", seenAt: new Date() },\n"
   "]);\n\n"
   "console.log(routes.get(\"cyan\")?.length);\n");
  launch(app);
  QVERIFY(waitJs(editor(),"window.dwcodeNativeReady===true && !!window.dwcodeEditor.getModel()"));
  app.backend->openFile("src/relay.ts");
  QVERIFY(waitJs(editor(),"window.dwcodeEditor.getModel().uri.path.endsWith('/relay.ts')"));
 }
 void capture(){
  theme("cyberpunk-chrome");
  // Language server: diagnostics in the editor and the problems panel, then a hover.
  click(withText("Connect LSP"));
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->lspReady(),20000);
  QTRY_VERIFY_WITH_TIMEOUT(!app.backend->problems().isEmpty(),20000);
  QVERIFY(waitJs(editor(),"document.querySelectorAll('.squiggly-error').length>0"));
  click(withText(QString("PROBLEMS (%1)").arg(app.backend->problems().size())));
  cursor(4,18);action("editor.action.showHover");
  QVERIFY(waitJs(editor(),"[...document.querySelectorAll('.monaco-hover-content')].some(n=>n.textContent.includes('route('))"));
  save("hero");
  js(editor(),"(window.dwcodeEditor.getContribution('editor.contrib.contentHover')._hideWidgets(),true)");
  QVERIFY(waitJs(editor(),"![...document.querySelectorAll('.monaco-hover-content')].some(n=>n.getBoundingClientRect().height>0)"));
  // Completion from the language server.
  cursor(21,1);QMetaObject::invokeMethod(editor(),"forceActiveFocus");typeText("routes.");
  QVERIFY(waitJs(editor(),"[...document.querySelectorAll('.suggest-widget .monaco-list-row')].some(n=>n.textContent.includes('forEach'))"));
  save("completion");
  action("hideSuggestWidget");js(editor(),"(window.dwcodeEditor.trigger('shots','undo',{}),true)");
  // Terminal running real commands in the workspace.
  click(withText("TERMINAL"));
  QQuickItem *field=nullptr;
  for(auto *i:items())if(i->inherits("QQuickTextField")&&i->isVisible()&&i->property("placeholderText").toString().startsWith("Run a command"))field=i;
  QVERIFY(field);
  app.backend->runCommand("ls -1 src && grep -n 'export' src/*.ts && wc -l src/*.ts");
  QTRY_VERIFY_WITH_TIMEOUT(app.backend->terminalOutput().contains("[exit 0]"),5000);
  click(field);typeText("echo signal-relay ready");
  save("terminal");
  QTest::keyClick(app.window,Qt::Key_Return);
  // Split view on the two source files.
  click(withText("OUTPUT"));
  click(withText("Split editor"));QTRY_VERIFY(app.window->property("splitVisible").toBool());
  QObject *second=nullptr;
  for(auto *v:app.window->findChildren<QObject*>())if(v->inherits("QQuickWebEngineView")&&v->objectName().isEmpty())second=v;
  QVERIFY(second);QVERIFY(waitJs(second,"window.dwcodeNativeReady===true && !!window.dwcodeEditor.getModel()"));
  js(second,"(window.dwcodeEditor.setPosition({lineNumber:8,column:1}),true)");
  save("split");
  click(withText("Single view"));QTRY_VERIFY(!app.window->property("splitVisible").toBool());
  // Theme chooser open over the editor.
  auto *picker=combo();click(picker);
  auto *popup=picker->property("popup").value<QObject*>();QTRY_VERIFY(popup->property("opened").toBool());
  save("themes");
  QTest::keyClick(app.window,Qt::Key_Escape);QTRY_VERIFY(!popup->property("opened").toBool());
  // The same file in a spread of palettes.
  click(withText("PROBLEMS ("+QString::number(app.backend->problems().size())+")"));
  for(const QString id:{"synthwave-84","tokyo-night","cyberdyne-t800","neosynth-plasma","dracula","cybercore.dwcode-frost"}){
   theme(id);cursor(1,1);save("theme-"+id);
  }
  theme("cyberpunk-chrome");
 }
 // Real-display layout pass: the settings dialog and the right-side Explorer.
 void layout(){
  auto *dialog=app.window->findChild<QObject*>("settingsDialog");QVERIFY(dialog);
  QVERIFY(QMetaObject::invokeMethod(dialog,"open"));QTRY_VERIFY(dialog->property("visible").toBool());
  save("settings");
  QQuickItem *search=nullptr;
  for(auto *i:items())if(i->inherits("QQuickTextField")&&i->property("placeholderText").toString().startsWith("Search settings"))search=i;
  QVERIFY(search);click(search);typeText("session");
  QTRY_COMPARE(dialog->property("query").toString(),QString("session"));
  save("settings-search");
  for(int i=0;i<7;++i)QTest::keyClick(app.window,Qt::Key_Backspace);
  QTRY_COMPARE(dialog->property("query").toString(),QString(""));
  // The extensions section: pack listing, licence metadata and the switch.
  click(search);typeText("extensions");
  QTRY_COMPARE(dialog->property("query").toString(),QString("extensions"));
  QTRY_VERIFY(withText("EXTENSIONS"));
  save("settings-extensions");
  for(int i=0;i<10;++i)QTest::keyClick(app.window,Qt::Key_Backspace);
  QTRY_COMPARE(dialog->property("query").toString(),QString(""));
  app.backend->setPreference("appearance/sidebarPosition","right");
  QVERIFY(QMetaObject::invokeMethod(dialog,"close"));QTRY_VERIFY(!dialog->property("visible").toBool());
  QTRY_VERIFY(app.window->property("sidebarRight").toBool());
  save("sidebar-right");
  app.backend->setPreference("appearance/sidebarPosition","left");
  // The command palette is the highest-traffic input; its prompt must read.
  app.window->setProperty("paletteVisible",true);
  QTRY_VERIFY(app.window->property("paletteVisible").toBool());
  save("command-palette");
  // Shortcuts only fire in an active window. A tool window is not always given
  // focus by the compositor, so fall back to the overlay's click-to-dismiss.
  app.window->requestActivate();
  if(app.window->isActive())
   QTest::keyClick(app.window,Qt::Key_Escape);
  else
   QTest::mouseClick(app.window,Qt::LeftButton,{},QPoint(app.window->width()/2,app.window->height()-40));
  QTRY_VERIFY(!app.window->property("paletteVisible").toBool());
 }
 void cleanupTestCase(){shutdown(app);}
};

int main(int argc,char **argv){
 prepareUiEnvironment();
 qputenv("TMPDIR","/tmp"); // keeps the home path out of the status bar
 if(qEnvironmentVariableIsEmpty("QT_SCALE_FACTOR"))qputenv("QT_SCALE_FACTOR","2");
 QtWebEngineQuick::initialize();
 QGuiApplication a(argc,argv);
 Shots s;
 // QTest treats argv[1] as a test function name, so pass only the program name.
 int one=1;
 return QTest::qExec(&s,one,argv);
}
#include "shots.moc"
