#include "MessageCodec.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QtGlobal>
#include <QtEndian>
#include <cstring>

namespace {
constexpr char kMagic[] = "ENC1";
constexpr int kMagicSize = 4;
constexpr int kNonceSize = 16;
constexpr int kMacSize = 32; // SHA-256

QByteArray sharedKey()
{
    static const QByteArray key = [](){
        QByteArray secret = qgetenv("CHAT_SHARED_KEY");
        if (secret.isEmpty()) {
            secret = QByteArrayLiteral("CHANGE_ME_SHARED_KEY");
        }
        return QCryptographicHash::hash(secret, QCryptographicHash::Sha256);
    }();
    return key;
}

QByteArray randomBytes(int size)
{
    QByteArray bytes(size, Qt::Uninitialized);
    auto *data = reinterpret_cast<uchar *>(bytes.data());
    for (int i = 0; i < size; ++i) {
        data[i] = static_cast<uchar>(QRandomGenerator::global()->generate() & 0xFF);
    }
    return bytes;
}

QByteArray keystreamBlock(const QByteArray &nonce, quint32 counter)
{
    QByteArray counterBytes(4, Qt::Uninitialized);
    qToBigEndian(counter, reinterpret_cast<uchar *>(counterBytes.data()));

    QByteArray input = sharedKey();
    input.append(nonce);
    input.append(counterBytes);
    return QCryptographicHash::hash(input, QCryptographicHash::Sha256);
}

QByteArray xorData(const QByteArray &input, const QByteArray &nonce)
{
    QByteArray output(input.size(), Qt::Uninitialized);
    int offset = 0;
    quint32 counter = 0;

    while (offset < input.size()) {
        QByteArray block = keystreamBlock(nonce, counter++);
        int chunk = qMin(block.size(), input.size() - offset);
        for (int i = 0; i < chunk; ++i) {
            output[offset + i] = static_cast<char>(
                static_cast<uchar>(input[offset + i]) ^ static_cast<uchar>(block[i])
            );
        }
        offset += chunk;
    }

    return output;
}

bool constantTimeEquals(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    uchar result = 0;
    for (int i = 0; i < a.size(); ++i) {
        result |= static_cast<uchar>(a[i] ^ b[i]);
    }
    return result == 0;
}

QByteArray encryptPayload(const QByteArray &plain)
{
    QByteArray nonce = randomBytes(kNonceSize);
    QByteArray cipher = xorData(plain, nonce);
    QByteArray mac = QMessageAuthenticationCode::hash(nonce + cipher, sharedKey(), QCryptographicHash::Sha256);

    QByteArray payload;
    payload.reserve(kMagicSize + kNonceSize + cipher.size() + mac.size());
    payload.append(kMagic, kMagicSize);
    payload.append(nonce);
    payload.append(cipher);
    payload.append(mac);
    return payload;
}

bool decryptPayload(const QByteArray &payload, QByteArray &plain)
{
    if (payload.size() < kMagicSize + kNonceSize + kMacSize) {
        return false;
    }
    if (memcmp(payload.constData(), kMagic, kMagicSize) != 0) {
        return false;
    }

    const int cipherOffset = kMagicSize + kNonceSize;
    const int cipherSize = payload.size() - cipherOffset - kMacSize;
    if (cipherSize < 0) {
        return false;
    }

    QByteArray nonce = payload.mid(kMagicSize, kNonceSize);
    QByteArray cipher = payload.mid(cipherOffset, cipherSize);
    QByteArray mac = payload.right(kMacSize);

    QByteArray expectedMac = QMessageAuthenticationCode::hash(nonce + cipher, sharedKey(), QCryptographicHash::Sha256);
    if (!constantTimeEquals(mac, expectedMac)) {
        return false;
    }

    plain = xorData(cipher, nonce);
    return true;
}
}

QByteArray MessageCodec::encode(const QByteArray &plain)
{
    QByteArray payload = encryptPayload(plain);
    QByteArray framed;
    framed.resize(static_cast<int>(sizeof(quint32)));
    qToBigEndian(static_cast<quint32>(payload.size()), reinterpret_cast<uchar *>(framed.data()));
    framed.append(payload);
    return framed;
}

MessageCodec::DecodeResult MessageCodec::decodeNext(QByteArray &buffer, QByteArray &outPlain)
{
    if (buffer.size() < static_cast<int>(sizeof(quint32))) {
        return DecodeResult::NeedMoreData;
    }

    quint32 payloadSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(buffer.constData()));
    if (buffer.size() < static_cast<int>(sizeof(quint32) + payloadSize)) {
        return DecodeResult::NeedMoreData;
    }

    QByteArray payload = buffer.mid(static_cast<int>(sizeof(quint32)), payloadSize);
    buffer.remove(0, static_cast<int>(sizeof(quint32) + payloadSize));

    if (!decryptPayload(payload, outPlain)) {
        return DecodeResult::Invalid;
    }

    return DecodeResult::Ok;
}
