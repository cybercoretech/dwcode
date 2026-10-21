#include "localwebserver.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DWCode"));
    app.setOrganizationName(QStringLiteral("Cybercore Tech"));
    app.setDesktopFileName(QStringLiteral("dwcode"));

    LocalWebServer editorServer(QStringLiteral(DWCODE_WEB_ROOT));
    if (!editorServer.start()) {
        qCritical() << "Could not start the local Monaco asset server:" << editorServer.errorString();
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("webEditorUrl"), editorServer.url());
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
