#ifndef DISPLAYFILTERENGINE_HPP
#define DISPLAYFILTERENGINE_HPP

#include <QObject>
#include <QString>
#include <pcap.h> // Cần cho pcap
#include "../../Common/PacketData.hpp" // Cần cho PacketData

class DisplayFilterEngine : public QObject
{
    Q_OBJECT
public:
    explicit DisplayFilterEngine(QObject *parent = nullptr);
    ~DisplayFilterEngine();

    /**
     * @brief Biên dịch một chuỗi filter BPF mới.
     * @param filterText Chuỗi filter (ví dụ: "tcp and host 192.168.1.1").
     * @return true nếu cú pháp hợp lệ, false nếu lỗi.
     */
    bool setFilter(const QString &filterText);

    /**
     * @brief Kiểm tra một gói tin có khớp với bộ lọc đã biên dịch không.
     * @param packet Gói tin (chứa raw_packet).
     * @return true nếu khớp.
     */
    bool packetMatches(const PacketData &packet) const;

    /**
     * @brief Lấy thông báo lỗi cú pháp gần nhất.
     */
    QString getLastError() const;

private:
    pcap_t* m_pcapHandle;      // Handle pcap "giả" (dummy) chỉ dùng để biên dịch
    bpf_program m_filterProgram; // Bộ lọc đã biên dịch
    bool m_filterIsSet;        // Cờ
    QString m_lastError;
};

#endif // DISPLAYFILTERENGINE_HPP
