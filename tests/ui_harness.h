// Shared driver for the real Main.qml window, used by the UI tests and the
// site screenshot tool. Input goes through the same paths a user's does.
#pragma once
#include "localwebserver.h"
#include "workspacebackend.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QIcon>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QSettings>
#include <QWheelEvent>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include <memory>
#include <functional>

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

class UiHarness:public QObject{
 Q_OBJECT
protected:
 QTemporaryDir settings,root;
 LocalWebServer server{QStringLiteral(DWCODE_WEB_ROOT)};
 App app;
 Probe probe;
 int nextId=0;
 QString workspace; // defaults to root


 static void put(const QString &path,const QByteArray &text){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(text),text.size());}
 static QByteArray get(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}

 void launch(App &a){
  a.backend=std::make_unique<WorkspaceBackend>();
  QVERIFY(a.backend->openWorkspace(workspace.isEmpty()?root.path():workspace));
  a.engine=std::make_unique<QQmlApplicationEngine>();
  a.engine->rootContext()->setContextProperty("workspaceBackend",a.backend.get());
  a.engine->rootContext()->setContextProperty("webEditorUrl",server.url());
  a.engine->globalObject().setProperty("probe",a.engine->newQObject(&probe));
  QQmlEngine::setObjectOwnership(&probe,QQmlEngine::CppOwnership);
  a.engine->load(QUrl("qrc:/qml/Main.qml"));
  QVERIFY(!a.engine->rootObjects().isEmpty());
  a.window=qobject_cast<QQuickWindow*>(a.engine->rootObjects().first());
  QVERIFY(a.window);
  a.window->setIcon(QIcon(":/qml/dwcode-icon.png"));
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
 // Isolated settings, local editor server, then the window on the workspace in root.
 void boot(const QString &appName){
  QCoreApplication::setOrganizationName("DWCodeTests");QCoreApplication::setApplicationName(appName);
  QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
  QVERIFY(server.start());
 }
};

// Never open a window on the developer's desktop; DWCODE_UI_PLATFORM overrides.
inline void prepareUiEnvironment(){
 qputenv("QT_QPA_PLATFORM",qEnvironmentVariableIsEmpty("DWCODE_UI_PLATFORM")?"offscreen":qgetenv("DWCODE_UI_PLATFORM"));
 qunsetenv("WAYLAND_DISPLAY");
 qputenv("QT_QPA_PLATFORMTHEME","");
 if(qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))qputenv("QT_QUICK_BACKEND","software");
 if(qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS"))qputenv("QTWEBENGINE_CHROMIUM_FLAGS","--disable-gpu");
}
