#pragma once
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QSet>
#include <QJsonObject>

class WorkspaceBackend final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString root READ root NOTIFY workspaceChanged)
    Q_PROPERTY(QVariantList files READ files NOTIFY workspaceChanged)
    Q_PROPERTY(QString activePath READ activePath NOTIFY documentStateChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY documentStateChanged)
    Q_PROPERTY(QVariantList documents READ documents NOTIFY documentStateChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QString terminalOutput READ terminalOutput NOTIFY terminalChanged)
    Q_PROPERTY(bool terminalRunning READ terminalRunning NOTIFY terminalChanged)
    Q_PROPERTY(QString lspStatus READ lspStatus NOTIFY lspChanged)
    Q_PROPERTY(bool lspReady READ lspReady NOTIFY lspChanged)
    Q_PROPERTY(QVariantList problems READ problems NOTIFY problemsChanged)
    Q_PROPERTY(QVariantList themes READ themes CONSTANT)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QVariantMap preferences READ preferences NOTIFY preferencesChanged)
public:
    explicit WorkspaceBackend(QObject *parent=nullptr);
    ~WorkspaceBackend() override;
    QString root() const { return m_root; }
    QVariantList files() const { return m_files; }
    QString activePath() const { return m_active; }
    bool dirty() const;
    QVariantList documents() const;
    QString message() const { return m_message; }
    QString terminalOutput() const { return m_terminalOutput; }
    bool terminalRunning() const { return m_terminal.state()!=QProcess::NotRunning; }
    QString lspStatus() const { return m_lspStatus; }
    bool lspReady() const { return m_lspReady; }
    QVariantList problems() const { return m_problems; }
    QVariantList themes() const { return m_themes; }
    QVariantMap theme() const { return m_theme; }
    QVariantMap preferences() const { return m_preferences; }
    Q_INVOKABLE bool openWorkspace(const QString &path);
    Q_INVOKABLE void refreshFiles();
    Q_INVOKABLE QVariantMap openFile(const QString &path);
    Q_INVOKABLE QVariantMap activeDocument() const;
    Q_INVOKABLE bool updateDocument(const QString &path,const QString &text);
    Q_INVOKABLE bool saveDocument(const QString &path);
    Q_INVOKABLE bool saveAs(const QString &url);
    Q_INVOKABLE bool hasUnsaved() const;
    Q_INVOKABLE void discardChanges();
    Q_INVOKABLE bool runCommand(const QString &command);
    Q_INVOKABLE void sendInput(const QString &text);
    Q_INVOKABLE void stopCommand();
    Q_INVOKABLE void clearTerminal();
    Q_INVOKABLE void startLanguageServer();
    Q_INVOKABLE void stopLanguageServer();
    Q_INVOKABLE int languageRequest(const QString &method,const QVariantMap &params);
    Q_INVOKABLE void setTheme(const QString &id);
    Q_INVOKABLE void setPreference(const QString &key,const QVariant &value);
    Q_INVOKABLE void resetPreferences();
signals:
    void workspaceChanged();
    void documentStateChanged();
    void documentActivated(const QVariantMap &document);
    void documentUpdated(const QVariantMap &document);
    void messageChanged();
    void terminalChanged();
    void lspChanged();
    void problemsChanged();
    void themeChanged();
    void preferencesChanged(const QVariantMap &preferences);
    void languageReply(int id,const QVariant &result,const QString &error);
    void diagnosticsReceived(const QString &uri,const QVariantList &diagnostics);
private:
    struct Document { QString path,text,language; QByteArray diskHash; int version=1; bool dirty=false; };
    QString resolve(const QString &path,bool newFile=false) const;
    QVariantMap describe(const Document &doc) const;
    void report(const QString &message);
    bool writeDocument(Document &doc,const QString &target,bool overwrite);
    void appendTerminal(const QString &text);
    void sendLsp(const QJsonObject &object);
    void notifyLsp(const QString &method,const QJsonObject &params);
    void readLsp();
    void handleLsp(const QJsonObject &message);
    void openInLsp(const Document &doc);
    bool supports(const Document &doc) const;
    QString preparedForSave(const QString &text) const;
    void loadPreferences();
    QString m_root,m_active,m_message="Open a folder to begin.",m_terminalOutput,m_lspStatus="Disconnected",m_lspFamily;
    QVariantList m_files,m_themes,m_problems;
    QVariantMap m_theme;
    QVariantMap m_preferences;
    QMap<QString,Document> m_documents;
    QMap<QString,QVariantList> m_diagnostics;
    QProcess m_terminal,m_lsp;
    QByteArray m_lspBuffer;
    bool m_lspReady=false;
    int m_requestId=100;
    QSet<QString> m_lspOpened;
    QSet<int> m_pending;
};
