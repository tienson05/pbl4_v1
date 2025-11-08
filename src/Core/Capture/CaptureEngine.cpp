#include "CaptureEngine.hpp"
#include "Parser.hpp"
#include <QThread>
#include <QRandomGenerator>
#include <QTime>
#include <QString>

CaptureEngine::CaptureEngine(QObject *parent)
    : QObject(parent)
    , m_isPaused(false)
    , m_isRunning(false)
{
}

CaptureEngine::~CaptureEngine() {
    stopCapture();
}
// Hàm set interface
void CaptureEngine::setInterface(const QString &interfaceName) {
    m_interface = interfaceName;
}
// Hàm set filter
void CaptureEngine::setCaptureFilter(const QString &filter) {
    m_captureFilter = filter;
}

void CaptureEngine::setDisplayFilter(const QString &filter) {
    m_displayFilter = filter;
}
// Hàm bắt đầu bắt gói tin
void CaptureEngine::startCapture() {
    if (m_isRunning) return;

    setupPcap();
    if (!m_pcapHandle) {
        emit errorOccurred(QString("Failed to open interface: %1").arg(m_errbuf));
        return;
    }
    if (!applyCaptureFilter()) {
        emit errorOccurred(QString("Failed to set filter: %1").arg(m_errbuf));
    }

    m_isRunning = true;
    m_isPaused = false;
    QThread* thread = QThread::create([this]() { captureLoop(); });
    thread->start();
}

void CaptureEngine::stopCapture() {
    m_isRunning = false;
}

void CaptureEngine::pauseCapture() {
    m_isPaused = true;
}

void CaptureEngine::resumeCapture() {
    m_isPaused = false;
}

void CaptureEngine::setupPcap() {
    closePcap();
    m_pcapHandle = pcap_open_live(m_interface.toUtf8().constData(),65536, 1, 1000, m_errbuf);
    if (!m_pcapHandle) {
        qDebug() << "pcap_open_live failed:" << m_errbuf;
    }
}

void CaptureEngine::closePcap() {
    if (m_pcapHandle) {
        pcap_close(m_pcapHandle);
        m_pcapHandle = nullptr;
    }
}

bool CaptureEngine::applyCaptureFilter() {
    if (m_captureFilter.isEmpty() || !m_pcapHandle) return true;

    struct bpf_program fp;
    if (pcap_compile(m_pcapHandle, &fp, m_captureFilter.toUtf8().constData(), 0, PCAP_NETMASK_UNKNOWN) == -1) {
        strncpy(m_errbuf, pcap_geterr(m_pcapHandle), PCAP_ERRBUF_SIZE);
        return false;
    }
    if (pcap_setfilter(m_pcapHandle, &fp) == -1) {
        pcap_freecode(&fp);
        strncpy(m_errbuf, pcap_geterr(m_pcapHandle), PCAP_ERRBUF_SIZE);
        return false;
    }
    pcap_freecode(&fp);
    return true;
}

void CaptureEngine::captureLoop() {
    while (m_isRunning && m_pcapHandle) {
        if (m_isPaused) {
            QThread::msleep(100);
            continue;
        }

        struct pcap_pkthdr* header;
        const u_char* packet;
        int ret = pcap_next_ex(m_pcapHandle, &header, &packet);

        if (ret == 1) {
            PacketData pkt;
            Parser parser;
            if (parser.parse(&pkt, packet, header->caplen)) {
                pkt.cap_length = header->caplen;
                pkt.wire_length = header->len;
                pkt.packet_id = ++m_packetCounter;

                emitPacket(pkt);
            }
        }
        else if (ret == -1) {
            emit errorOccurred(QString("pcap error: %1").arg(pcap_geterr(m_pcapHandle)));
            break;
        }
    }
}

void CaptureEngine::emitPacket(const PacketData& pkt) {
    QMetaObject::invokeMethod(this, [this, pkt]() {
        emit packetCaptured(pkt);
    }, Qt::QueuedConnection);
}
