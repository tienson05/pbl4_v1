#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QThread>
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
    void startCapture();
    void stopCapture(); // (Hàm này giờ sẽ "chờ")
    void pauseCapture();
    void resumeCapture();
    bool isPaused() const { return m_isPaused; }

signals:

    void packetsCaptured(QList<PacketData>* packetBatch);
    void errorOccurred(const QString &error);

private:
    void captureLoop();
    void fileReadingLoop();

    // --- pcap ---
    pcap_t* m_pcapHandle = nullptr;
    char m_errbuf[PCAP_ERRBUF_SIZE]{};

    // --- config ---
    QString m_interface;
    QString m_captureFilter;

    // --- state ---
    volatile bool m_isPaused = false;
    volatile bool m_isRunning = false;
    int m_packetCounter = 0;

    QThread* m_captureThread = nullptr; // Con trỏ theo dõi luồng

    // --- helper ---
    bool setupPcap();
    void closePcap();
    bool applyCaptureFilter();
};
