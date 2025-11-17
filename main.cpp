#include <QCoreApplication>
#include <QBluetoothSocket>
#include <QBluetoothLocalDevice>
#include <QBluetoothUuid>
#include <QBluetoothAddress>
#include <QTimer>
#include <QDateTime>
#include <iostream>
#include "packets.h"
#include "media/mediacontroller.h"

QString getTimestamp() {
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}

class AirPodsHandoff : public QObject {
    Q_OBJECT

public:
    AirPodsHandoff(const QString &airpodsMac, QObject *parent = nullptr)
        : QObject(parent), airpodsMac(airpodsMac)
    {
        // Get local Bluetooth MAC for comparison
        QBluetoothLocalDevice localDevice;
        QBluetoothAddress localAddr = localDevice.address();
        localMac = QByteArray::fromHex(localAddr.toString().replace(":", "").toLatin1());
        std::reverse(localMac.begin(), localMac.end());

        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Local MAC (reversed): " << localMac.toHex().toStdString() << std::endl;

        // Initialize media controller
        QString deviceMac = QString(airpodsMac).replace(":", "_");
        media = new MediaController(deviceMac, this);
        connect(media, &MediaController::playbackStarted, this, &AirPodsHandoff::onPlaybackStarted);

        // Connect to AirPods
        connectToAirPods();
    }

private slots:
    void onConnected() {
        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Connected to AirPods" << std::endl;

        // Send handshake
        socket->write(Packets::Connection::HANDSHAKE);

        // Don't request notifications yet - wait for FEATURES_ACK
    }

    void onDataReceived() {
        QByteArray data = socket->readAll();

        // Debug: log all received data (commented out - uncomment for debugging)
        // if (data.size() > 0) {
        //     std::cout << "[Handoff] Received packet: " << data.toHex().toStdString() << std::endl;
        // }

        // Handle FEATURES_ACK - send REQUEST_NOTIFICATIONS after receiving this
        if (data.startsWith(Packets::Connection::FEATURES_ACK)) {
            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Received FEATURES_ACK - requesting notifications" << std::endl;
            socket->write(Packets::Connection::REQUEST_NOTIFICATIONS);
            return;
        }

        // Parse AUDIO_SOURCE packets
        if (data.startsWith(Packets::AudioSource::HEADER)) {
            auto newSource = Packets::AudioSource::parse(data);
            if (newSource.isValid) {
                QString typeStr = (newSource.type == Packets::AudioSource::NONE) ? "NONE" :
                                (newSource.type == Packets::AudioSource::CALL) ? "CALL" : "MEDIA";
                std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Audio source: " << newSource.deviceMac.toHex().toStdString()
                         << " (" << typeStr.toStdString() << ")" << std::endl;

                // Check if another device took audio from us
                bool weHadAudio = currentSource.isValid &&
                                 currentSource.type != Packets::AudioSource::NONE &&
                                 currentSource.deviceMac == localMac;

                bool otherDeviceHasAudio = newSource.type != Packets::AudioSource::NONE &&
                                          newSource.deviceMac != localMac;

                // Handle NONE: if another device took audio from us and then released it, reclaim
                if (newSource.type == Packets::AudioSource::NONE) {
                    if (shouldReclaimOnNone) {
                        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Another device released audio - reclaiming" << std::endl;

                        if (socket && socket->isOpen()) {
                            socket->write(Packets::OwnsConnection::CLAIM);
                            media->reclaimAudioStream();
                        }

                        shouldReclaimOnNone = false;  // Reset flag
                    }
                }
                // Another device has audio
                else if (otherDeviceHasAudio) {
                    // If we had audio and another device took it, pause and mark for reclaim
                    if (weHadAudio) {
                        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Another device took audio from us - pausing Linux" << std::endl;
                        media->pauseAllMedia();
                        shouldReclaimOnNone = true;  // Reclaim when they release
                    }
                    // If Linux has any active audio (MPRIS or Discord/games), mark for reclaim
                    else if (media->isMediaPlaying() || media->hasActiveAudio()) {
                        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Another device has audio and Linux has active audio - marking for reclaim" << std::endl;
                        media->pauseAllMedia();  // Try to pause MPRIS players if any
                        shouldReclaimOnNone = true;  // Reclaim when they release
                    }
                }

                // Remember last non-NONE source
                if (newSource.type != Packets::AudioSource::NONE) {
                    currentSource = newSource;
                }
            }
        }
    }

    void onPlaybackStarted() {
        // If socket is not connected, we can't do handoff - just try to force reclaim audio
        if (!socket || !socket->isOpen()) {
            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Playback started but socket disconnected - forcing audio reclaim" << std::endl;
            media->reclaimAudioStream();
            return;
        }

        // Check if we need to reclaim audio
        if (currentSource.isValid) {
            if (currentSource.type == Packets::AudioSource::NONE) {
                std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Playback started - no device has audio" << std::endl;
                // Proactively claim ownership
                socket->write(Packets::OwnsConnection::CLAIM);
                return;
            }

            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Comparing MACs - Current: " << currentSource.deviceMac.toHex().toStdString()
                     << ", Local: " << localMac.toHex().toStdString() << std::endl;

            if (currentSource.deviceMac == localMac) {
                std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] We already own audio, no handoff needed" << std::endl;
                return;
            }

            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Another device has audio - reclaiming" << std::endl;
        } else {
            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Playback started - no AUDIO_SOURCE info yet, claiming proactively" << std::endl;
        }

        // Claim ownership and reclaim audio stream
        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Sending OWNS_CONNECTION (claim)" << std::endl;
        socket->write(Packets::OwnsConnection::CLAIM);

        // Try to reclaim via suspend/resume (fallback to profile cycling if needed)
        media->reclaimAudioStream();
    }

    void onDisconnected() {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Handoff] Socket disconnected!" << std::endl;

        // Clear state since we can't get updates anymore
        currentSource = Packets::AudioSource::Info();
        shouldReclaimOnNone = false;

        // Try to reconnect after a short delay
        QTimer::singleShot(2000, this, [this]() {
            std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Attempting to reconnect..." << std::endl;
            connectToAirPods();
        });
    }

    void onError(QBluetoothSocket::SocketError error) {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Handoff] Socket error: " << static_cast<int>(error) << std::endl;
    }

private:
    void connectToAirPods() {
        std::cout << "[" << getTimestamp().toStdString() << "] [Handoff] Connecting to AirPods..." << std::endl;

        // Clean up old socket if it exists
        if (socket) {
            socket->disconnect();
            socket->deleteLater();
        }

        socket = new QBluetoothSocket(QBluetoothServiceInfo::L2capProtocol, this);

        connect(socket, &QBluetoothSocket::connected, this, &AirPodsHandoff::onConnected);
        connect(socket, &QBluetoothSocket::readyRead, this, &AirPodsHandoff::onDataReceived);
        connect(socket, &QBluetoothSocket::disconnected, this, &AirPodsHandoff::onDisconnected);
        connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::errorOccurred),
                this, &AirPodsHandoff::onError);

        // Connect to AirPods AACP service
        QBluetoothAddress addr(airpodsMac);
        socket->connectToService(addr, QBluetoothUuid("74ec2172-0bad-4d01-8f77-997b2be0722a"));
    }

    QString airpodsMac;
    QByteArray localMac;
    QBluetoothSocket *socket = nullptr;
    MediaController *media = nullptr;
    Packets::AudioSource::Info currentSource;
    bool shouldReclaimOnNone = false;  // Set to true when another device takes audio from us
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <AirPods_MAC_Address>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 34:0E:22:49:C4:73" << std::endl;
        return 1;
    }

    QString airpodsMac = QString(argv[1]);

    std::cout << "=== AirPods Seamless Handoff ===" << std::endl;
    std::cout << "[" << getTimestamp().toStdString() << "] [Main] AirPods MAC: " << airpodsMac.toStdString() << std::endl;

    AirPodsHandoff handoff(airpodsMac);

    return app.exec();
}

#include "main.moc"
