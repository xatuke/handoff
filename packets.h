#ifndef PACKETS_H
#define PACKETS_H

#include <QByteArray>

namespace Packets {
    // AACP Control Command - OWNS_CONNECTION
    namespace OwnsConnection {
        static const QByteArray HEADER = QByteArray::fromHex("040004000900");

        static QByteArray createCommand(quint8 identifier, quint8 data1) {
            QByteArray packet = HEADER;
            packet.append(static_cast<char>(identifier));
            packet.append(static_cast<char>(data1));
            packet.append(static_cast<char>(0x00));
            packet.append(static_cast<char>(0x00));
            packet.append(static_cast<char>(0x00));
            return packet;
        }

        static const QByteArray CLAIM = createCommand(0x06, 0x01);
        static const QByteArray RELEASE = createCommand(0x06, 0x00);
    }

    // TiPi Protocol - Audio Source (tells which device is playing)
    namespace AudioSource {
        static const QByteArray HEADER = QByteArray::fromHex("040004000E");

        enum Type : quint8 {
            NONE = 0x00,
            CALL = 0x01,
            MEDIA = 0x02
        };

        struct Info {
            QByteArray deviceMac;
            Type type;
            bool isValid;
        };

        inline Info parse(const QByteArray &data) {
            Info info{QByteArray(), NONE, false};

            // Format: 04 00 04 00 0E [1 byte] [6 bytes MAC] [1 byte type]
            if (data.size() >= 13 && data.startsWith(HEADER)) {
                info.deviceMac = data.mid(6, 6);
                info.type = static_cast<Type>(static_cast<quint8>(data.at(12)));
                info.isValid = true;
            }

            return info;
        }
    }

    // Connection handshake packets
    namespace Connection {
        static const QByteArray HANDSHAKE = QByteArray::fromHex("00000400010002000000000000000000");
        static const QByteArray REQUEST_NOTIFICATIONS = QByteArray::fromHex("040004000f00ffffffffff");
        static const QByteArray FEATURES_ACK = QByteArray::fromHex("040004002b00");
    }
}

#endif // PACKETS_H
