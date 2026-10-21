#include "localwebserver.h"

#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QTcpSocket>

LocalWebServer::LocalWebServer(QString rootDirectory, QObject *parent)
    : QObject(parent), m_rootDirectory(QFileInfo(std::move(rootDirectory)).canonicalFilePath())
{
    connect(&m_server, &QTcpServer::newConnection, this, &LocalWebServer::handleConnection);
}

bool LocalWebServer::start()
{
    return !m_rootDirectory.isEmpty() && m_server.listen(QHostAddress::LocalHost, 0);
}

QString LocalWebServer::errorString() const
{
    if (m_rootDirectory.isEmpty())
        return QStringLiteral("The bundled editor assets were not built.");
    return m_server.errorString();
}

QUrl LocalWebServer::url() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(m_server.serverPort()));
}

void LocalWebServer::handleConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        const auto readRequest = [this, socket] {
            QByteArray request = socket->property("dwcode.request").toByteArray();
            request.append(socket->readAll());
            if (!request.contains("\r\n\r\n")) {
                socket->setProperty("dwcode.request", request);
                return;
            }
            socket->setProperty("dwcode.request", QVariant());
            serveRequest(socket, request);
        };
        connect(socket, &QTcpSocket::readyRead, this, readRequest);
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        if (socket->bytesAvailable() > 0)
            readRequest();
    }
}

void LocalWebServer::serveRequest(QTcpSocket *socket, const QByteArray &request)
{
    const QByteArray requestLine = request.left(request.indexOf('\n')).trimmed();
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 2 || requestParts.at(0) != "GET") {
        socket->write("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    const QUrl requestUrl = QUrl::fromEncoded(requestParts.at(1));
    QString relativePath = requestUrl.path(QUrl::FullyDecoded);
    if (relativePath.isEmpty() || relativePath == QStringLiteral("/"))
        relativePath = QStringLiteral("/index.html");
    relativePath = QUrl::fromPercentEncoding(relativePath.toUtf8());

    const QString candidate = QFileInfo(m_rootDirectory + relativePath).canonicalFilePath();
    const QString rootPrefix = m_rootDirectory + QLatin1Char('/');
    QString resolvedPath = candidate;
    if (candidate.isEmpty() || (!candidate.startsWith(rootPrefix) && candidate != m_rootDirectory))
        resolvedPath.clear();

    if (relativePath == QStringLiteral("/qwebchannel.js")) resolvedPath = QStringLiteral(":/qtwebchannel/qwebchannel.js");
    QFile file(resolvedPath);
    if (resolvedPath.isEmpty() || !file.open(QIODevice::ReadOnly) || !QFileInfo(file).isFile()) {
        socket->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    const QByteArray body = file.readAll();
    const QByteArray headers = "HTTP/1.1 200 OK\r\nContent-Type: " + mimeTypeFor(resolvedPath)
        + "\r\nContent-Length: " + QByteArray::number(body.size())
        + "\r\nCache-Control: no-cache\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n";
    socket->write(headers);
    socket->write(body);
    socket->disconnectFromHost();
}

QByteArray LocalWebServer::mimeTypeFor(const QString &path) const
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("html")) return "text/html; charset=utf-8";
    if (suffix == QStringLiteral("js")) return "text/javascript; charset=utf-8";
    if (suffix == QStringLiteral("css")) return "text/css; charset=utf-8";
    if (suffix == QStringLiteral("json")) return "application/json; charset=utf-8";
    if (suffix == QStringLiteral("svg")) return "image/svg+xml";
    if (suffix == QStringLiteral("woff2")) return "font/woff2";
    if (suffix == QStringLiteral("woff")) return "font/woff";
    if (suffix == QStringLiteral("ttf")) return "font/ttf";
    if (suffix == QStringLiteral("png")) return "image/png";
    if (suffix == QStringLiteral("map")) return "application/json";
    return "application/octet-stream";
}
