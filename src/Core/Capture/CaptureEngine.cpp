#include "CaptureEngine.hpp"
#include "Parser.hpp"
#include <QThread>
#include <QRandomGenerator>
#include <QTime>
#include <QString>
#include <QMetaObject>

const int LIVE_CAPTURE_TIMEOUT_MS = 100; // Gửi lô sau mỗi 100ms
const int LIVE_BATCH_SIZE = 200;         // Hoặc khi đủ 200 gói
const int FILE_READ_BATCH_SIZE = 1000;   // Gửi 1000 gói/lần khi đọc file

CaptureEngine::CaptureEngine(QObject *parent)
    : QObject(parent)
    , m_isPaused(false)
    , m_isRunning(false)
{
}

CaptureEngine::~CaptureEngine() {
    stopCapture();
}

void CaptureEngine::setInterface(const QString &interfaceName) {
    m_interface = interfaceName;
}

void CaptureEngine::setCaptureFilter(const QString &filter) {
    m_captureFilter = filter;
}


void CaptureEngine::startCapture() {
    if (m_isRunning) stopCapture(); // Dừng luồng cũ nếu đang chạy

    m_isRunning = true;
    m_isPaused = false;
    m_packetCounter = 0;

    m_captureThread = QThread::create([this]() {
        captureLoop(); // Hàm này sẽ chạy trên luồng mới
    });
    connect(m_captureThread, &QThread::finished, m_captureThread, &QObject::deleteLater);
    m_captureThread->start();
}


void CaptureEngine::stopCapture() {
    m_isRunning = false; // Báo cho các vòng lặp dừng lại

    if (m_pcapHandle) {
        // Ngắt (break) vòng lặp pcap_next_ex ngay lập tức
        pcap_breakloop(m_pcapHandle);
    }

    // Chờ cho luồng (thread) cũ kết thúc VÀ bị xóa
    if (m_captureThread && m_captureThread->isRunning()) {
        m_captureThread->quit(); // Yêu cầu thoát
        m_captureThread->wait(1000); // Chờ (block) tối đa 1 giây
    }
    m_captureThread = nullptr; // Xóa con trỏ cũ
    // -----------------------
}

void CaptureEngine::pauseCapture() {
    m_isPaused = true;
}

void CaptureEngine::resumeCapture() {
    m_isPaused = false;
}

bool CaptureEngine::setupPcap() {
    closePcap();
    m_pcapHandle = pcap_open_live(m_interface.toUtf8().constData(), 65536, 1, LIVE_CAPTURE_TIMEOUT_MS, m_errbuf);
    if (!m_pcapHandle) {
        qDebug() << "pcap_open_live failed:" << m_errbuf;
        return false;
    }
    pcap_setnonblock(m_pcapHandle, 1, m_errbuf);
    return true;
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
    if (!setupPcap()) {
        emit errorOccurred(QString("Failed to open interface: %1").arg(m_errbuf));
        return;
    }
    if (!applyCaptureFilter()) {
        emit errorOccurred(QString("Failed to set filter: %1").arg(m_errbuf));
        closePcap();
        return;
    }

    Parser parser;
    QList<PacketData>* packetBatch = new QList<PacketData>();
    packetBatch->reserve(LIVE_BATCH_SIZE);
    struct pcap_pkthdr* header;
    const u_char* data;
    int ret;

    while (m_isRunning)
    {
        if (m_isPaused) {
            QThread::msleep(100);
            continue;
        }

        ret = pcap_next_ex(m_pcapHandle, &header, &data);

        if (ret == 1) {
            PacketData pkt;
            if (parser.parse(&pkt, data, header->caplen)) {
                pkt.cap_length = header->caplen;
                pkt.wire_length = header->len;
                pkt.packet_id = ++m_packetCounter;
                packetBatch->append(pkt);
            }
        }
        else if (ret == 0) { // Timeout
            // (Bỏ qua, vòng lặp sẽ kiểm tra logic gửi lô)
        }
        else if (ret == -1 || ret == -2) { // Lỗi hoặc break
            qDebug() << "pcap_next_ex error or breakloop";
            break;
        }

        if ( (packetBatch->size() >= LIVE_BATCH_SIZE) ||
            (ret == 0 && !packetBatch->isEmpty()) )
        {
            QMetaObject::invokeMethod(this, [this, packetBatch]() {
                emit packetsCaptured(packetBatch); // Gửi con trỏ
            }, Qt::QueuedConnection);
            packetBatch = new QList<PacketData>();
            packetBatch->reserve(LIVE_BATCH_SIZE);
        }
    } // Kết thúc while(m_isRunning)

    if (!packetBatch->isEmpty()) {
        QMetaObject::invokeMethod(this, [this, packetBatch]() {
            emit packetsCaptured(packetBatch);
        }, Qt::QueuedConnection);
    } else {
        delete packetBatch;
    }

    closePcap();
    qDebug() << "Capture thread finished.";
}


void CaptureEngine::startCaptureFromFile(const QString &filePath)
{
    if (m_isRunning) stopCapture();
    m_interface = filePath;
    m_captureFilter = "";
    m_isRunning = true;
    m_isPaused = false;
    m_packetCounter = 0;

    // (SỬA) Gán luồng mới vào biến thành viên
    m_captureThread = QThread::create([this]() { fileReadingLoop(); });
    connect(m_captureThread, &QThread::finished, m_captureThread, &QObject::deleteLater);
    m_captureThread->start();
}

void CaptureEngine::fileReadingLoop()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    m_pcapHandle = pcap_open_offline(m_interface.toStdString().c_str(), errbuf);
    if (!m_pcapHandle) {
        emit errorOccurred(QString("pcap_open_offline error: %1").arg(errbuf));
        return;
    }

    struct pcap_pkthdr* header;
    const u_char* data;
    int res;
    Parser parser;
    QList<PacketData>* packetBatch = new QList<PacketData>();
    packetBatch->reserve(FILE_READ_BATCH_SIZE);

    while (m_isRunning && (res = pcap_next_ex(m_pcapHandle, &header, &data)) >= 0)
    {
        if (res == 1) {
            PacketData pkt;
            if (parser.parse(&pkt, data, header->caplen)) {
                pkt.cap_length = header->caplen;
                pkt.wire_length = header->len;
                pkt.packet_id = ++m_packetCounter;
                packetBatch->append(pkt);
            }

            if (packetBatch->size() >= FILE_READ_BATCH_SIZE)
            {
                QMetaObject::invokeMethod(this, [this, packetBatch]() {
                    emit packetsCaptured(packetBatch);
                }, Qt::QueuedConnection);
                packetBatch = new QList<PacketData>();
                packetBatch->reserve(FILE_READ_BATCH_SIZE);
            }
        }
        else if (res == -2) { // Hết file
            break;
        }
    } // Kết thúc while

    if (!packetBatch->isEmpty()) {
        QMetaObject::invokeMethod(this, [this, packetBatch]() {
            emit packetsCaptured(packetBatch);
        }, Qt::QueuedConnection);
    } else {
        delete packetBatch;
    }

    closePcap();
    qDebug() << "File reading thread finished.";
}
