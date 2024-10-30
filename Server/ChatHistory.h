#ifndef CHATHISTORY_H
#define CHATHISTORY_H

#include <QObject>
#include <QString>

class ChatHistory : public QObject
{
    Q_OBJECT
public:
    explicit ChatHistory(QObject *parent = nullptr);

    void logTextMessage(const QString &sender, const QString &receiver, const QString &message);
    QString historyFilePath() const;

private:
    QString _historyFilePath;
};

#endif // CHATHISTORY_H
