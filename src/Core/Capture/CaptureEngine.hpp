#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <pcap.h>
#include "../../Common/PacketData.hpp"

class CaptureEngine : public QObject {
    Q_OBJECT
public:
    explicit CaptureEngine(QObject *parent = nullptr);
    ~CaptureEngine();

    void setInterface(const QString &interfaceName);
    void setCaptureFilter(const QString &filter);
    void startCaptureFromFile(const QString &filePath);
    void setDisplayFilter(const QString &filter);
    void startCapture();
    void stopCapture();
    void pauseCapture();
    void resumeCapture();
    bool isPaused() const { return m_isPaused; }

signals:
    void packetCaptured(const PacketData &packet);
    void errorOccurred(const QString &error);

private slots:
    void captureLoop();
    void fileReadingLoop();

private:
    // --- pcap ---
    pcap_t* m_pcapHandle = nullptr;
    char m_errbuf[PCAP_ERRBUF_SIZE]{};

    // --- config ---
    QString m_interface;
    QString m_captureFilter;
    QString m_displayFilter;

    // --- state ---
    volatile bool m_isPaused = false;
    volatile bool m_isRunning = false;
    int m_packetCounter = 0;

    // --- helper ---
    void setupPcap();
    void closePcap();
    bool applyCaptureFilter();
    void emitPacket(const PacketData& pkt);
};
