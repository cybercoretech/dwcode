#include "packloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QPair>
#include <algorithm>

#ifndef DWCODE_VERSION
#define DWCODE_VERSION "0.1.0"
#endif

namespace PackLoader {
namespace {

const QStringList surfaceKeys{QStringLiteral("window"),QStringLiteral("shell"),QStringLiteral("sidebar"),
                              QStringLiteral("editor"),QStringLiteral("raised"),QStringLiteral("border")};
const QStringList textKeys{QStringLiteral("primary"),QStringLiteral("secondary"),QStringLiteral("muted")};
const QStringList accentKeys{QStringLiteral("primary"),QStringLiteral("secondary"),QStringLiteral("success"),
                             QStringLiteral("warning"),QStringLiteral("error")};
const QStringList syntaxKeys{QStringLiteral("comment"),QStringLiteral("keyword"),QStringLiteral("string"),
                             QStringLiteral("number"),QStringLiteral("type"),QStringLiteral("function"),
                             QStringLiteral("variable"),QStringLiteral("constant")};
const QStringList knownContributions{QStringLiteral("themes"),QStringLiteral("snippets"),QStringLiteral("iconTheme")};

bool readJson(const QString &path,QJsonObject *out,QString *error){
 QFile file(path);
 if(!file.open(QIODevice::ReadOnly)){*error=QStringLiteral("cannot read ")+QFileInfo(path).fileName();return false;}
 const QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
 if(doc.isNull()||!doc.isObject()){*error=QFileInfo(path).fileName()+QStringLiteral(" is not a JSON object");return false;}
 *out=doc.object();
 return true;
}

// Colors are DWCode theme tokens: #RRGGBB with an optional alpha pair.
bool isColor(const QJsonValue &value){
 static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6}(?:[0-9a-fA-F]{2})?$"));
 return value.isString()&&pattern.match(value.toString()).hasMatch();
}

// Rejects anything that escapes the pack directory, on disk and in resources.
bool resolveInside(const QString &directory,const QString &relative,QString *out){
 if(relative.isEmpty())return false;
 // Joining by hand keeps :/ resources working; QDir::absoluteFilePath would
 // prefix them with the process working directory.
 QString base=QDir::cleanPath(directory);
 if(!base.endsWith(QLatin1Char('/')))base+=QLatin1Char('/');
 const QString root=base.chopped(1);
 const QString candidate=QDir::cleanPath(base+relative);
 if(!candidate.startsWith(root+QLatin1Char('/')))return false;
 if(!QFileInfo::exists(candidate))return false;
 // On disk, also reject symlink escapes; resources have no symlinks to follow.
 const QString realRoot=QFileInfo(root).canonicalFilePath();
 const QString realCandidate=QFileInfo(candidate).canonicalFilePath();
 if(!realRoot.isEmpty()&&realCandidate!=realRoot&&!realCandidate.startsWith(realRoot+QLatin1Char('/')))return false;
 *out=candidate;
 return true;
}

struct Version{int major=0,minor=0,patch=0;};

bool parseVersion(const QString &text,Version *out){
 static const QRegularExpression pattern(QStringLiteral("^v?(\\d+)\\.(\\d+)(?:\\.(\\d+))?"));
 const auto match=pattern.match(text.trimmed());
 if(!match.hasMatch())return false;
 out->major=match.captured(1).toInt();
 out->minor=match.captured(2).toInt();
 out->patch=match.captured(3).isEmpty()?0:match.captured(3).toInt();
 return true;
}

int compare(const Version &a,const Version &b){
 if(a.major!=b.major)return a.major<b.major?-1:1;
 if(a.minor!=b.minor)return a.minor<b.minor?-1:1;
 if(a.patch!=b.patch)return a.patch<b.patch?-1:1;
 return 0;
}

Version upperBound(const Version &version,bool caret){
 // ^1.2.3 -> <2.0.0, ^0.2.3 -> <0.3.0, ^0.0.3 -> <0.0.4
 // ~1.2.3 -> <1.3.0
 Version bound=version;
 if(caret){
  if(version.major>0){bound.major=version.major+1;bound.minor=0;bound.patch=0;}
  else if(version.minor>0){bound.minor=version.minor+1;bound.patch=0;}
  else bound.patch=version.patch+1;
 }else{
  bound.minor=version.minor+1;bound.patch=0;
 }
 return bound;
}

QString contributionSummary(const Pack &pack){
 QStringList bits;
 if(!pack.themes.isEmpty())
  bits<<QStringLiteral("%1 theme%2").arg(pack.themes.size()).arg(pack.themes.size()==1?QString():QStringLiteral("s"));
 if(!pack.snippets.isEmpty()){
  QStringList languages=pack.snippets.keys();std::sort(languages.begin(),languages.end());
  bits<<QStringLiteral("snippets: ")+languages.join(QStringLiteral(", "));
 }
 if(!pack.iconTheme.isEmpty())
  bits<<QStringLiteral("icons (not rendered in the Explorer yet)");
 if(!pack.unsupported.isEmpty())
  bits<<QStringLiteral("not consumed yet: ")+pack.unsupported.join(QStringLiteral(", "));
 return bits.join(QStringLiteral("  ·  "));
}

}

QString bundledDirectory(){return QStringLiteral(":/packs");}
QString userDirectory(){
 const QFileInfo info(QSettings().fileName());
 return info.absolutePath()+QLatin1Char('/')+info.completeBaseName()+QStringLiteral(".packs");
}
QString appVersion(){return QStringLiteral(DWCODE_VERSION);}

QStringList packDirectories(){
 QStringList out;
 const QStringList roots{bundledDirectory(),userDirectory()};
 for(const auto &root:roots){
  const QDir dir(root);
  if(!root.startsWith(QLatin1Char(':'))&&!dir.exists())continue;
  if(QFileInfo::exists(root+QStringLiteral("/manifest.json"))){out<<root;continue;}
  QStringList entries=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
  std::sort(entries.begin(),entries.end());
  for(const auto &entry:entries)
   if(QFileInfo::exists(root+QLatin1Char('/')+entry+QStringLiteral("/manifest.json")))
    out<<root+QLatin1Char('/')+entry;
 }
 return out;
}
QStringList loadOrder(){return packDirectories();}

bool versionSatisfies(const QString &range,const QString &version){
 if(range.trimmed().isEmpty())return false; // no declared support window: fail closed
 Version current;
 if(!parseVersion(version,&current))return false;
 const auto terms=range.simplified().split(QLatin1Char(' '),Qt::SkipEmptyParts);
 for(const auto &term:terms){
  QString operatorText=term, operand=term;
  if(term.startsWith(QLatin1String(">="))){operatorText=QStringLiteral(">=");operand=term.mid(2);}
  else if(term.startsWith(QLatin1String("<="))){operatorText=QStringLiteral("<=");operand=term.mid(2);}
  else if(term.startsWith(QLatin1Char('>'))){operatorText=QStringLiteral(">");operand=term.mid(1);}
  else if(term.startsWith(QLatin1Char('<'))){operatorText=QStringLiteral("<");operand=term.mid(1);}
  else if(term.startsWith(QLatin1Char('^'))){operatorText=QStringLiteral("^");operand=term.mid(1);}
  else if(term.startsWith(QLatin1Char('~'))){operatorText=QStringLiteral("~");operand=term.mid(1);}
  else if(term.startsWith(QLatin1Char('='))){operatorText=QStringLiteral("=");operand=term.mid(1);}
  else operatorText=QStringLiteral("=");
  Version wanted;
  if(!parseVersion(operand,&wanted))return false;
  const int relation=compare(current,wanted);
  if(operatorText==QStringLiteral(">=")){if(relation<0)return false;}
  else if(operatorText==QStringLiteral("<=")){if(relation>0)return false;}
  else if(operatorText==QStringLiteral(">")){if(relation<=0)return false;}
  else if(operatorText==QStringLiteral("<")){if(relation>=0)return false;}
  else if(operatorText==QStringLiteral("=")){if(relation!=0)return false;}
  else if(operatorText==QStringLiteral("^")||operatorText==QStringLiteral("~")){
   if(relation<0)return false;
   if(compare(current,upperBound(wanted,operatorText==QStringLiteral("^")))>=0)return false;
  }else return false;
 }
 return true;
}

Pack load(const QString &directory){
 Pack pack;
 pack.directory=directory;
 pack.source=directory.startsWith(QLatin1Char(':'))?QStringLiteral("bundled"):QStringLiteral("user");
 pack.id=QFileInfo(directory).fileName();

 QJsonObject manifest;QString error;
 if(!readJson(directory+QStringLiteral("/manifest.json"),&manifest,&error)){
  pack.errors<<error;return pack;
 }
 pack.id=manifest.value(QStringLiteral("id")).toString(pack.id);
 pack.displayName=manifest.value(QStringLiteral("displayName")).toString(pack.id);
 pack.description=manifest.value(QStringLiteral("description")).toString();
 pack.version=manifest.value(QStringLiteral("version")).toString();
 pack.license=manifest.value(QStringLiteral("license")).toString();
 pack.publisher=manifest.value(QStringLiteral("publisher")).toString();

 if(manifest.value(QStringLiteral("manifestVersion")).toInt()!=1)
  pack.errors<<QStringLiteral("manifestVersion must be 1");
 if(manifest.value(QStringLiteral("kind")).toString()!=QStringLiteral("declarative"))
  pack.errors<<QStringLiteral("kind must be \"declarative\"");
 if(pack.license.trimmed().isEmpty())
  pack.errors<<QStringLiteral("license metadata is required");
 if(manifest.contains(QStringLiteral("main"))||manifest.contains(QStringLiteral("browser")))
  pack.errors<<QStringLiteral("executable entry points are not allowed in declarative packs");
 if(!manifest.value(QStringLiteral("capabilities")).toArray().isEmpty())
  pack.errors<<QStringLiteral("declarative packs cannot request capabilities");
 if(!versionSatisfies(manifest.value(QStringLiteral("engines")).toObject().value(QStringLiteral("dwcode")).toString(),appVersion()))
  pack.errors<<QStringLiteral("not compatible with DWCode ")+appVersion();

 const QJsonObject contributes=manifest.value(QStringLiteral("contributes")).toObject();
 for(auto it=contributes.constBegin();it!=contributes.constEnd();++it)
  if(!knownContributions.contains(it.key()))
   pack.unsupported<<it.key();

 for(const auto &value:contributes.value(QStringLiteral("themes")).toArray()){
  QString path;
  if(!resolveInside(directory,value.toString(),&path)){
   pack.errors<<QStringLiteral("theme resource escapes the pack: ")+value.toString();continue;
  }
  QJsonObject themeJson;QString themeError;
  if(!readJson(path,&themeJson,&themeError)){pack.errors<<themeError;continue;}
  if(themeJson.value(QStringLiteral("schemaVersion")).toInt()!=1){
   pack.errors<<value.toString()+QStringLiteral(" must use schemaVersion 1");continue;
  }
  if(themeJson.value(QStringLiteral("id")).toString().isEmpty()||themeJson.value(QStringLiteral("name")).toString().isEmpty()){
   pack.errors<<value.toString()+QStringLiteral(" needs an id and name");continue;
  }
  const QPair<QString,QStringList> groups[]={{
    QStringLiteral("surfaces"),surfaceKeys},{QStringLiteral("text"),textKeys},
    {QStringLiteral("accents"),accentKeys},{QStringLiteral("syntax"),syntaxKeys}};
  bool ok=true;
  for(const auto &group:groups){
   const QJsonObject object=themeJson.value(group.first).toObject();
   if(object.isEmpty()){
    pack.errors<<value.toString()+QStringLiteral(" is missing ")+group.first+QStringLiteral(" tokens");ok=false;continue;
   }
   for(const auto &key:group.second)
    if(!isColor(object.value(key))){
     pack.errors<<value.toString()+QStringLiteral(" has an invalid ")+group.first+QLatin1Char('/')+key+QStringLiteral(" color");ok=false;
    }
  }
  if(ok)pack.themes<<themeJson.toVariantMap();
 }

 const QJsonObject snippets=contributes.value(QStringLiteral("snippets")).toObject();
 for(auto it=snippets.constBegin();it!=snippets.constEnd();++it){
  QString path;
  if(!resolveInside(directory,it.value().toString(),&path)){
   pack.errors<<QStringLiteral("snippet resource escapes the pack: ")+it.value().toString();continue;
  }
  QJsonObject entries;QString snippetError;
  if(!readJson(path,&entries,&snippetError)){pack.errors<<snippetError;continue;}
  QVariantList list;
  for(auto entry=entries.constBegin();entry!=entries.constEnd();++entry){
   const QJsonObject snippet=entry.value().toObject();
  const QJsonValue prefix=snippet.value(QStringLiteral("prefix"));
  const QJsonValue body=snippet.value(QStringLiteral("body"));
   const QString first=prefix.isArray()?prefix.toArray().first().toString():prefix.toString();
   if(first.isEmpty()||body.toString().isEmpty()&&body.toArray().isEmpty()){
    pack.errors<<path+QStringLiteral(" entry “")+entry.key()+QStringLiteral("” needs a prefix and body");continue;
   }
   QStringList lines;
   if(body.isArray()){for(const auto &line:body.toArray())lines<<line.toString();}
   else lines<<body.toString();
   list<<QVariantMap{{QStringLiteral("name"),entry.key()},{QStringLiteral("prefix"),first},
                     {QStringLiteral("body"),lines.join(QLatin1Char('\n'))},
                     {QStringLiteral("description"),snippet.value(QStringLiteral("description")).toString()}};
  }
  if(!list.isEmpty())pack.snippets.insert(it.key(),list);
 }

 const QString iconTheme=contributes.value(QStringLiteral("iconTheme")).toString();
 if(!iconTheme.isEmpty()){
  QString path;
  if(!resolveInside(directory,iconTheme,&path))
   pack.errors<<QStringLiteral("icon theme resource escapes the pack: ")+iconTheme;
  else pack.iconTheme=iconTheme;
 }

 pack.valid=pack.errors.isEmpty();
 return pack;
}

QVector<Pack> loadAll(){
 QVector<Pack> out;
 for(const auto &directory:loadOrder()) out<<load(directory);
 return out;
}

QVariantMap toVariant(const Pack &pack){
 QStringList errors;for(const auto &message:pack.errors)errors<<QStringLiteral("Rejected: ")+message;
 return QVariantMap{
  {QStringLiteral("id"),pack.id},{QStringLiteral("name"),pack.displayName},
  {QStringLiteral("description"),pack.description},{QStringLiteral("version"),pack.version},
  {QStringLiteral("license"),pack.license},{QStringLiteral("publisher"),pack.publisher},
  {QStringLiteral("source"),pack.source},{QStringLiteral("directory"),pack.directory},
  {QStringLiteral("valid"),pack.valid},{QStringLiteral("errors"),errors},
  {QStringLiteral("errorText"),errors.join(QLatin1Char('\n'))},
  {QStringLiteral("note"),contributionSummary(pack)},
  {QStringLiteral("themes"),pack.themes},{QStringLiteral("snippets"),pack.snippets},
 };
}

}
