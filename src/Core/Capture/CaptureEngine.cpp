#include "CaptureEngine.hpp"
#include "Parser.hpp"
#include <QThread>
#include <QDebug>
#include <QList>
#include <chrono> // <-- THÊM MỚI: Cần cho việc đếm thời gian batch

CaptureEngine::CaptureEngine(QObject *parent)
    : QObject(parent),
    m_isPaused(false),
    m_isRunning(false),
    m_packetCounter(0)
{
}

CaptureEngine::~CaptureEngine() {
    stopCapture();
    // (Đã xóa closePcap() ở đây, QThread sẽ xử lý)
}
// Hàm set interface
void CaptureEngine::setInterface(const QString &interfaceName) {
    m_interface = interfaceName;
}
// Hàm set filter
void CaptureEngine::setCaptureFilter(const QString &filter) {
    m_captureFilter = filter;
}

// (Hàm setDisplayFilter đã bị xóa)

// Hàm bắt đầu bắt gói tin
void CaptureEngine::startCapture() {
    if (m_isRunning) {
        // Nếu đang chạy, chỉ cần khởi động lại
        stopCapture(); // Dừng luồng cũ
    }

    // (Không setupPcap() ở đây, hãy để luồng mới tự làm)

    m_isRunning = true;
    m_isPaused = false;
    m_packetCounter = 0; // Reset bộ đếm

    // Tạo và bắt đầu một luồng mới, chỉ định nó chạy captureLoop
    QThread* thread = QThread::create([this]() {
        captureLoop(); // Hàm này sẽ chạy trên luồng mới
    });

    // Tự động xóa QThread khi nó kết thúc
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void CaptureEngine::stopCapture() {
    m_isRunning = false; // Báo cho vòng lặp dừng lại

    // Ngắt (break) vòng lặp pcap_next_ex ngay lập tức
    if (m_pcapHandle) {
        pcap_breakloop(m_pcapHandle);
    }

    // (Không cần chờ (wait) ở đây, tín hiệu/khe (signal/slot) sẽ xử lý)
}

void CaptureEngine::pauseCapture() {
    m_isPaused = true;
}

void CaptureEngine::resumeCapture() {
    m_isPaused = false;
}

// (setupPcap, closePcap, applyCaptureFilter vẫn giữ nguyên
//  nhưng chúng sẽ được gọi TỪ BÊN TRONG luồng)
void CaptureEngine::setupPcap() {
    closePcap();
    // (Timeout 500ms là quan trọng để batching hoạt động mượt)
    m_pcapHandle = pcap_open_live(m_interface.toUtf8().constData(), 65536, 1, 500, m_errbuf);
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
    // (Hàm này giữ nguyên như code cũ của bạn)
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

/**
 * @brief (ĐÃ THAY ĐỔI) Hàm bắt gói tin chính,
 * hiện đã có logic "Batching" (Gom lô).
 */
void CaptureEngine::captureLoop() {

    // --- 1. Cài đặt (Setup) pcap TRONG LUỒNG NÀY ---
    setupPcap();
    if (!m_pcapHandle) {
        emit errorOccurred(QString("Failed to open interface: %1").arg(m_errbuf));
        return;
    }
    if (!applyCaptureFilter()) {
        emit errorOccurred(QString("Failed to set filter: %1").arg(m_errbuf));
        closePcap();
        return;
    }

    struct pcap_pkthdr* header;
    const u_char* data;
    int res;

    // --- 2. LOGIC BATCHING MỚI ---
    QList<PacketData> packetBuffer;
    const int BATCH_SIZE = 50; // Gửi 50 gói một lần
    const int FLUSH_TIMEOUT_MS = 100; // Hoặc gửi sau mỗi 100ms
    auto lastFlushTime = std::chrono::steady_clock::now();
    // ----------------------------

    Parser parser; // Tạo parser MỘT lần bên ngoài vòng lặp

    // 3. Vòng lặp chính
    while (m_isRunning && (res = pcap_next_ex(m_pcapHandle, &header, &data)) >= 0)
    {
        while (m_isPaused && m_isRunning) {
            QThread::msleep(100); // Ngủ nếu đang Tạm dừng
        }
        if (!m_isRunning) break; // Thoát nếu Dừng

        if (res == 1) { // 1. Bắt được gói tin
            PacketData pkt;

            // 2. Phân tích (Parse)
            if (parser.parse(&pkt, data, header->caplen)) {
                pkt.cap_length = header->caplen;
                pkt.wire_length = header->len;
                pkt.packet_id = ++m_packetCounter;

                // 3. Thêm vào bộ đệm (buffer)
                packetBuffer.append(pkt);
            }
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlushTime).count();

        // 4. Kiểm tra xem có cần GỬI (flush) bộ đệm không
        if (!packetBuffer.isEmpty() && (packetBuffer.size() >= BATCH_SIZE || elapsed >= FLUSH_TIMEOUT_MS))
        {
            // Phát tín hiệu MỘT LẦN với MỘT LÔ gói tin
            // Dùng Qt::QueuedConnection để gửi an toàn qua các luồng
            QMetaObject::invokeMethod(this, [this, packetBuffer]() {
                emit packetsCaptured(packetBuffer);
            }, Qt::QueuedConnection);

            // Xóa bộ đệm và reset thời gian
            packetBuffer.clear();
            lastFlushTime = now;
        }
        else if (res == 0) // pcap_next_ex bị timeout
        {
            // Không làm gì, chỉ để vòng lặp tiếp tục
            // và kiểm tra m_isRunning / m_isPaused
        }
    } // Kết thúc while

    // Gửi nốt các gói tin còn sót lại trong bộ đệm
    if (!packetBuffer.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, packetBuffer]() {
            emit packetsCaptured(packetBuffer);
        }, Qt::QueuedConnection);
    }

    closePcap(); // Dọn dẹp handle
}

