#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

#include <QString>
#include <QProcess>
#include <QRegularExpression>

class PulseAudio {
public:
    static bool setProfile(const QString &cardName, const QString &profileName) {
        QProcess process;
        process.start("pactl", QStringList() << "set-card-profile" << cardName << profileName);
        process.waitForFinished();
        return process.exitCode() == 0;
    }

    static QString getCardForDevice(const QString &macAddress) {
        QProcess process;
        process.start("pactl", QStringList() << "list" << "cards" << "short");
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');

        for (const QString &line : lines) {
            if (line.contains(macAddress)) {
                QStringList parts = line.split('\t');
                if (parts.size() >= 2) {
                    return parts[1]; // Card name
                }
            }
        }
        return QString();
    }

    static QString getSinkForDevice(const QString &macAddress) {
        QProcess process;
        process.start("pactl", QStringList() << "list" << "sinks" << "short");
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');

        for (const QString &line : lines) {
            // Match bluez sinks containing the MAC address
            if (line.contains("bluez") && line.contains(macAddress)) {
                // Format: INDEX\tNAME\tDRIVER\tFORMAT\tSTATE
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1]; // Sink name (second column)
                }
            }
        }
        return QString();
    }

    static bool suspendSink(const QString &sinkName, bool suspend) {
        QProcess process;
        process.start("pactl", QStringList() << "suspend-sink" << sinkName << (suspend ? "1" : "0"));
        process.waitForFinished();
        return process.exitCode() == 0;
    }

    static QString getSinkIndex(const QString &sinkName) {
        QProcess process;
        process.start("pactl", QStringList() << "list" << "sinks" << "short");
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');

        for (const QString &line : lines) {
            if (line.contains(sinkName)) {
                QStringList parts = line.split('\t');
                if (parts.size() >= 1) {
                    return parts[0]; // Sink index
                }
            }
        }
        return QString();
    }

    static bool hasActiveAudio(const QString &sinkName) {
        // First get the sink index for our AirPods sink
        QString sinkIndex = getSinkIndex(sinkName);
        if (sinkIndex.isEmpty()) {
            return false;
        }

        QProcess process;
        process.start("pactl", QStringList() << "list" << "sink-inputs");
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');

        QString currentSinkIndex;
        bool isCorked = true;

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();

            // Check if this sink-input is connected to our sink (by index)
            if (trimmed.startsWith("Sink:")) {
                QStringList parts = trimmed.split(' ');
                if (parts.size() >= 2) {
                    currentSinkIndex = parts[1];
                }
            }

            // Check if it's NOT corked (corked = paused)
            if (trimmed.startsWith("Corked:")) {
                if (trimmed.contains("no")) {
                    isCorked = false;
                } else {
                    isCorked = true;
                }

                // If we found our sink and it's not corked, we have active audio
                if (currentSinkIndex == sinkIndex && !isCorked) {
                    return true;
                }

                // Reset for next sink-input
                currentSinkIndex.clear();
                isCorked = true;
            }
        }

        return false;
    }
};

#endif // PULSEAUDIO_H
