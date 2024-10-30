#ifndef MESSAGECODEC_H
#define MESSAGECODEC_H

#include <QByteArray>

class MessageCodec
{
public:
    enum class DecodeResult {
        Ok,
        NeedMoreData,
        Invalid
    };

    static QByteArray encode(const QByteArray &plain);
    static DecodeResult decodeNext(QByteArray &buffer, QByteArray &outPlain);
};

#endif // MESSAGECODEC_H
