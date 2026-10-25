#include "localwebserver.h"
#include "workspacebackend.h"
#include <QTimer>
#include <QQuickWindow>
#include <QDir>
#include <QImage>
#include <QIcon>
#include <QCommandLineOption>
#include <QCommandLineParser>

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
    app.setApplicationVersion(QStringLiteral(DWCODE_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/qml/dwcode-icon.png")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("DWCode keyboard-first desktop editor"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption workspaceOption(
        {QStringLiteral("w"), QStringLiteral("workspace")},
        QStringLiteral("Open a workspace folder."),
        QStringLiteral("folder"));
    const QCommandLineOption captureOption(
        QStringLiteral("capture-dir"),
        QStringLiteral("Capture prototype screenshots for project automation."),
        QStringLiteral("directory"));
    parser.addOption(workspaceOption);
    parser.addOption(captureOption);
    parser.process(app);

    LocalWebServer editorServer(QStringLiteral(DWCODE_WEB_ROOT));
    if (!editorServer.start()) {
        qCritical() << "Could not start the local Monaco asset server:" << editorServer.errorString();
        return 1;
    }

    WorkspaceBackend workspace;
    if (parser.isSet(workspaceOption))
        workspace.openWorkspace(parser.value(workspaceOption));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("workspaceBackend"), &workspace);
    engine.rootContext()->setContextProperty(QStringLiteral("webEditorUrl"), editorServer.url());
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (parser.isSet(captureOption)) {
        const QString destination = parser.value(captureOption);
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
