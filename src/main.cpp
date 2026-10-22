#include "localwebserver.h"
#include "workspacebackend.h"
#include <QTimer>
#include <QQuickWindow>
#include <QDir>
#include <QImage>
#include <QIcon>

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
    app.setWindowIcon(QIcon(QStringLiteral(":/qml/dwcode-icon.png")));

    LocalWebServer editorServer(QStringLiteral(DWCODE_WEB_ROOT));
    if (!editorServer.start()) {
        qCritical() << "Could not start the local Monaco asset server:" << editorServer.errorString();
        return 1;
    }

    WorkspaceBackend workspace;
    const auto args = app.arguments();
    const int workspaceFlag = args.indexOf("--workspace");
    if (workspaceFlag >= 0 && workspaceFlag + 1 < args.size()) workspace.openWorkspace(args.at(workspaceFlag + 1));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("workspaceBackend"), &workspace);
    engine.rootContext()->setContextProperty(QStringLiteral("webEditorUrl"), editorServer.url());
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    const int captureFlag = args.indexOf("--capture-dir");
    if (captureFlag >= 0 && captureFlag + 1 < args.size()) {
        const QString destination = args.at(captureFlag + 1);
        QDir().mkpath(destination);
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(7000, &app, [&, window, destination] {
            window->grabWindow().save(destination + "/workspace.png");
            window->setProperty("splitVisible", true);
            QTimer::singleShot(3500, &app, [&, window, destination] {
                window->grabWindow().save(destination + "/split-editor.png");
                window->setProperty("paletteVisible", true);
                QTimer::singleShot(500, &app, [&, window, destination] {
                    window->grabWindow().save(destination + "/command-palette.png");
                    app.quit();
                });
            });
        });
    }
    return app.exec();
}
