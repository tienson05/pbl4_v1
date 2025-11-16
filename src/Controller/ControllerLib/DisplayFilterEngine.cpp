#include "DisplayFilterEngine.hpp"
#include <QDebug>

DisplayFilterEngine::DisplayFilterEngine(QObject *parent)
    : QObject(parent), m_filterIsSet(false)
{
    // Tạo một pcap handle "giả" (dead)
    // Chúng ta dùng nó chỉ để truy cập trình biên dịch BPF
    m_pcapHandle = pcap_open_dead(DLT_EN10MB, 65535);
}

DisplayFilterEngine::~DisplayFilterEngine()
{
    if (m_pcapHandle) {
        pcap_close(m_pcapHandle);
    }
    if (m_filterIsSet) {
        pcap_freecode(&m_filterProgram);
    }
}

QString DisplayFilterEngine::getLastError() const
{
    return m_lastError;
}

bool DisplayFilterEngine::setFilter(const QString &filterText)
{
    // 1. Nếu filter rỗng, chỉ cần "tắt" bộ lọc
    if (filterText.isEmpty()) {
        if (m_filterIsSet) {
            pcap_freecode(&m_filterProgram);
        }
        m_filterIsSet = false;
        m_lastError.clear();
        return true;
    }

    // 2. Giải phóng bộ lọc cũ (nếu có)
    if (m_filterIsSet) {
        pcap_freecode(&m_filterProgram);
        m_filterIsSet = false;
    }

    // 3. Biên dịch (compile) bộ lọc mới
    // (PCAP_NETMASK_UNKNOWN rất quan trọng)
    if (pcap_compile(m_pcapHandle, &m_filterProgram,
                     filterText.toStdString().c_str(),
                     1, PCAP_NETMASK_UNKNOWN) == -1)
    {
        // Lỗi cú pháp
        m_lastError = pcap_geterr(m_pcapHandle);
        qDebug() << "BPF Display Filter Error:" << m_lastError;
        return false;
    }

    // 4. Biên dịch thành công
    m_filterIsSet = true;
    m_lastError.clear();
    return true;
}

bool DisplayFilterEngine::packetMatches(const PacketData &packet) const
{
    // 1. Nếu không có filter nào được đặt, tất cả đều khớp
    if (!m_filterIsSet) {
        return true;
    }

    // 2. Tạo một pcap_pkthdr "giả"
    struct pcap_pkthdr header;
    header.caplen = packet.cap_length;
    header.len = packet.wire_length;
    // (Timestamp không quan trọng cho việc lọc)

    // 3. Chạy bộ lọc BPF "offline"
    // Đây là hàm cốt lõi: nó chạy bộ lọc đã biên dịch
    // trên dữ liệu thô (raw_packet)
    int result = pcap_offline_filter(
        &m_filterProgram,
        &header,
        packet.raw_packet.data()
        );

    // 'result' > 0 có nghĩa là "khớp"
    return (result > 0);
}
