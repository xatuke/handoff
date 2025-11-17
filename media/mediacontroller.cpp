#include "mediacontroller.h"

MediaController::MediaController(const QString &deviceMac, QObject *parent)
    : QObject(parent), deviceMac(deviceMac)
{
    cardName = PulseAudio::getCardForDevice(deviceMac);
    sinkName = PulseAudio::getSinkForDevice(deviceMac);
    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Card name: " << cardName.toStdString() << std::endl;
    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Sink name: " << sinkName.toStdString() << std::endl;

    // Monitor MPRIS for media playback state changes
    QDBusConnection::sessionBus().connect(
        "",
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
}

void MediaController::reclaimAudioStream()
{
    if (sinkName.isEmpty()) {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Media] No sink name, falling back to profile cycling" << std::endl;
        cycleProfiles();
        return;
    }

    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Attempting to reclaim audio via suspend/resume" << std::endl;

    // Suspend the sink (sends AVDTP SUSPEND)
    if (PulseAudio::suspendSink(sinkName, true)) {
        std::cout << "[" << getTimestamp().toStdString() << "] [Media] Sink suspended" << std::endl;
    } else {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Media] Failed to suspend, trying profile cycle" << std::endl;
        cycleProfiles();
        return;
    }

    QThread::msleep(200);

    // Resume the sink (sends AVDTP START)
    if (PulseAudio::suspendSink(sinkName, false)) {
        std::cout << "[" << getTimestamp().toStdString() << "] [Media] Sink resumed - handoff complete" << std::endl;
    } else {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Media] Failed to resume, trying profile cycle" << std::endl;
        cycleProfiles();
    }
}

void MediaController::cycleProfiles()
{
    if (cardName.isEmpty()) {
        std::cerr << "[" << getTimestamp().toStdString() << "] [Media] No card name, cannot cycle profiles" << std::endl;
        return;
    }

    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Cycling profiles: HFP -> A2DP" << std::endl;

    // Switch to HFP
    PulseAudio::setProfile(cardName, "handsfree_head_unit");
    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Switched to HFP" << std::endl;

    QThread::msleep(200);

    // Switch to A2DP
    PulseAudio::setProfile(cardName, "a2dp_sink");
    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Switched to A2DP - handoff complete" << std::endl;
}

void MediaController::pauseAllMedia()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    QStringList services = bus.interface()->registeredServiceNames().value();

    int pausedCount = 0;
    for (const QString &service : services) {
        if (!service.startsWith("org.mpris.MediaPlayer2.")) continue;

        QDBusInterface iface(service, "/org/mpris/MediaPlayer2",
                           "org.mpris.MediaPlayer2.Player", bus);
        if (!iface.isValid()) continue;

        QDBusReply<void> reply = iface.call("Pause");
        if (reply.isValid()) {
            pausedCount++;
            std::cout << "[" << getTimestamp().toStdString() << "] [Media] Paused: " << service.toStdString() << std::endl;
        }
    }

    if (pausedCount > 0) {
        std::cout << "[" << getTimestamp().toStdString() << "] [Media] Paused " << pausedCount << " player(s)" << std::endl;
    }
}

bool MediaController::isMediaPlaying()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    QStringList services = bus.interface()->registeredServiceNames().value();

    for (const QString &service : services) {
        if (!service.startsWith("org.mpris.MediaPlayer2.")) continue;

        QDBusInterface iface(service, "/org/mpris/MediaPlayer2",
                           "org.mpris.MediaPlayer2.Player", bus);
        if (!iface.isValid()) continue;

        QVariant status = iface.property("PlaybackStatus");
        if (status.isValid() && status.toString() == "Playing") {
            return true;
        }
    }
    return false;
}

bool MediaController::hasActiveAudio()
{
    if (sinkName.isEmpty()) {
        std::cout << "[" << getTimestamp().toStdString() << "] [Media] No sink name, can't check for active audio" << std::endl;
        return false;
    }

    bool hasAudio = PulseAudio::hasActiveAudio(sinkName);
    std::cout << "[" << getTimestamp().toStdString() << "] [Media] Checking for active audio on sink " << sinkName.toStdString()
              << " -> " << (hasAudio ? "YES" : "NO") << std::endl;
    return hasAudio;
}

void MediaController::onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
{
    // Only handle MPRIS Player interface changes
    if (interface != "org.mpris.MediaPlayer2.Player") {
        return;
    }

    // Check if PlaybackStatus changed
    if (changed.contains("PlaybackStatus")) {
        QString status = changed.value("PlaybackStatus").toString();
        std::cout << "[" << getTimestamp().toStdString() << "] [Media] Playback status changed to: " << status.toStdString() << std::endl;

        if (status == "Playing") {
            std::cout << "[" << getTimestamp().toStdString() << "] [Media] Detected playback started!" << std::endl;
            emit playbackStarted();
        }
    }
}
