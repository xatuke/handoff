#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QThread>
#include <QStringList>
#include <QVariantMap>
#include <iostream>
#include "pulseaudio.h"

class MediaController : public QObject {
    Q_OBJECT

public:
    explicit MediaController(const QString &deviceMac, QObject *parent = nullptr);

    // Cycle profiles to force audio stream reclaim (fallback method)
    void cycleProfiles();

    // Try to reclaim audio by suspending/resuming the sink
    void reclaimAudioStream();

    // Pause all playing media
    void pauseAllMedia();

    // Check if any media is currently playing
    bool isMediaPlaying();

    // Check if there's any active audio (including non-MPRIS apps like Discord)
    bool hasActiveAudio();

signals:
    void playbackStarted();

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    QString deviceMac;
    QString cardName;
    QString sinkName;
};

#endif // MEDIACONTROLLER_H
