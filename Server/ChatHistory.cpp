#include "ChatHistory.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

ChatHistory::ChatHistory(QObject *parent)
    : QObject(parent)
{
    auto baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::currentPath() + "/history";
    }
    QDir().mkpath(baseDir);
    _historyFilePath = baseDir + "/chat_history.jsonl";
}

void ChatHistory::logTextMessage(const QString &sender, const QString &receiver, const QString &message)
{
    QJsonObject entry;
    entry["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    entry["sender"] = sender;
    entry["receiver"] = receiver;
    entry["message"] = message;

    QFile file(_historyFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }

    QJsonDocument doc(entry);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.write("\n");
}

QString ChatHistory::historyFilePath() const
{
    return _historyFilePath;
}
