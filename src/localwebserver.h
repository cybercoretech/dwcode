#pragma once

#include <QObject>
#include <QTcpServer>
#include <QUrl>

class QTcpSocket;

class LocalWebServer final : public QObject
{
    Q_OBJECT
public:
    explicit LocalWebServer(QString rootDirectory, QObject *parent = nullptr);
    bool start();
    QString errorString() const;
    QUrl url() const;

private:
    void handleConnection();
    void serveRequest(QTcpSocket *socket, const QByteArray &request);
    QByteArray mimeTypeFor(const QString &path) const;

    QString m_rootDirectory;
    QTcpServer m_server;
};
