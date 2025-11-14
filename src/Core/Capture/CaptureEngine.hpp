#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QList> // <-- THÊM MỚI
#include <pcap.h>
#include "../../Common/PacketData.hpp"

class CaptureEngine : public QObject {
    Q_OBJECT
public:
    explicit CaptureEngine(QObject *parent = nullptr);
    ~CaptureEngine();

    void setInterface(const QString &interfaceName);
    void setCaptureFilter(const QString &filter);
    void startCapture();
    void stopCapture();
    void pauseCapture();
    void resumeCapture();
    bool isPaused() const { return m_isPaused; }

signals:
    /**
     * @brief (ĐÃ THAY ĐỔI) Phát ra một LÔ gói tin thay vì 1 gói
     */
    void packetsCaptured(const QList<PacketData> &packets); // <-- ĐÃ THAY ĐỔI

    void errorOccurred(const QString &error);

private slots:
    void captureLoop(); // Hàm này sẽ được QThread gọi

private:
    // --- pcap ---
    pcap_t* m_pcapHandle = nullptr;
    char m_errbuf[PCAP_ERRBUF_SIZE]{};

    // --- config ---
    QString m_interface;
    QString m_captureFilter;

    // --- state ---
    volatile bool m_isPaused = false;  // <-- THÊM volatile
    volatile bool m_isRunning = false; // <-- THÊM volatile
    int m_packetCounter = 0;

    // --- helper ---
    void setupPcap();
    void closePcap();
    bool applyCaptureFilter();
};
