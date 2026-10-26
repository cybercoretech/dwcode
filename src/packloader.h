#pragma once
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

// Loads and validates declarative extension packs (EXTENSIONS.md stage 2).
// The C++ checks mirror tools/validate-packs.mjs: the running app never trusts
// a pack merely because CI validated it, and every failure is reported rather
// than skipped silently.
namespace PackLoader {

struct Pack {
    QString directory;    // absolute path or :/ resource path
    QString source;       // "bundled" (ships with DWCode) or "user"
    QString id, displayName, description, version, license, publisher;
    QVariantList themes;  // validated DWCode theme objects
    QVariantMap snippets; // language -> [{name,prefix,body,description}]
    QString iconTheme;    // relative path; recorded but not rendered yet
    QStringList unsupported; // contribution keys DWCode cannot consume yet
    QStringList errors;
    bool valid = false;
};

QString bundledDirectory();
QString userDirectory();
QStringList packDirectories();      // directories that contain a manifest.json
QStringList loadOrder();            // bundled first, then user packs
QString appVersion();
bool versionSatisfies(const QString &range, const QString &version);
Pack load(const QString &directory);
QVector<Pack> loadAll();
QVariantMap toVariant(const Pack &pack);

}
