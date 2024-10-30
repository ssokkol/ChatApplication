#include "ClientManager.h"

ClientManager::ClientManager(QHostAddress ip, ushort port, QObject *parent)
    : QObject{parent},
      _ip(ip),
      _port(port)
{
    setupClient();
}

void ClientManager::connectToServer()
{
    _socket->connectToHost(_ip, _port);
}

void ClientManager::sendMessage(QString message, QString receiver)
{
    sendPacket(_protocol.textMessage(message, receiver));
}

void ClientManager::sendName(QString name)
{
    sendPacket(_protocol.setNameMessage(name));
}

void ClientManager::sendStatus(ChatProtocol::Status status)
{
    sendPacket(_protocol.setStatusMessage(status));
}

void ClientManager::sendIsTyping()
{
    sendPacket(_protocol.isTypingMessage());
}

void ClientManager::sendInitSendingFile(QString fileName)
{
    _tmpFileName = fileName;
    sendPacket(_protocol.setInitSendingFileMessage(fileName));
}

void ClientManager::sendAcceptFile()
{
    sendPacket(_protocol.setAcceptFileMessage());

}

void ClientManager::sendRejectFile()
{
    sendPacket(_protocol.setRejectFileMessage());

}

void ClientManager::readyRead()
{
    _recvBuffer.append(_socket->readAll());
    while (true) {
        QByteArray plain;
        auto result = MessageCodec::decodeNext(_recvBuffer, plain);
        if (result == MessageCodec::DecodeResult::NeedMoreData) {
            break;
        }
        if (result == MessageCodec::DecodeResult::Invalid) {
            _recvBuffer.clear();
            break;
        }

        _protocol.loadData(plain);
        switch (_protocol.type()) {
        case ChatProtocol::Text:
            emit textMessageReceived(_protocol.message());
            break;
        case ChatProtocol::SetName:
            emit nameChanged(_protocol.name());
            break;
        case ChatProtocol::SetStatus:
            emit statusChanged(_protocol.status());
            break;
        case ChatProtocol::IsTyping:
            emit isTyping();
            break;
        case ChatProtocol::InitSendingFile:
            emit initReceivingFile(_protocol.name(), _protocol.fileName(), _protocol.fileSize());
            break;
        case ChatProtocol::AcceptSendingFile:
            sendFile();
            break;
        case ChatProtocol::RejectSendingFile:
            emit rejectReceivingFile();
            break;
        case ChatProtocol::ConnectionACK:
            emit connectionACK(_protocol.myName(), _protocol.clientsName());
            break;
        case ChatProtocol::NewClient:
            emit newClientConnectedToServer(_protocol.clientName());
            break;
        case ChatProtocol::ClientDisconnected:
            emit clientDisconnected(_protocol.clientName());
            break;
        case ChatProtocol::ClientName:
            emit clientNameChanged(_protocol.prevName(), _protocol.clientName());
            break;
        default:
            break;
        }
    }
}

void ClientManager::setupClient()
{
    _socket = new QTcpSocket(this);
    connect(_socket, &QTcpSocket::connected, this, &ClientManager::connected);
    connect(_socket, &QTcpSocket::disconnected, this, &ClientManager::disconnected);
    connect(_socket, &QTcpSocket::readyRead, this, &ClientManager::readyRead);
}

void ClientManager::sendFile()
{
    sendPacket(_protocol.setFileMessage(_tmpFileName));
}

void ClientManager::sendPacket(const QByteArray &payload)
{
    _socket->write(MessageCodec::encode(payload));
}


