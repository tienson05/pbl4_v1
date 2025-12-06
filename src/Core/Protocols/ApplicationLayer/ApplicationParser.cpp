#include "ApplicationParser.hpp"
#include "HTTPParser.hpp"
#include "DNSParser.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring> // Cần cho memcpy

static std::string hexStr(const uint8_t* data, size_t len) {
    if (len == 0) return "";
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}
// ---------------------------------------------------------

// --- HÀM HELPER: Parse SSDP (Port 1900) ---
static bool parseSSDP(const uint8_t* data, size_t len, std::string& infoOutput) {
    if (len == 0) return false;

    // Chuyển data sang string để kiểm tra
    std::string content(reinterpret_cast<const char*>(data), len);

    // Kiểm tra các từ khóa đặc trưng của SSDP
    if (content.find("HTTP/1.1") == std::string::npos &&
        content.find("M-SEARCH") == std::string::npos &&
        content.find("NOTIFY") == std::string::npos) {
        return false;
    }

    // Lấy dòng đầu tiên làm Info
    std::stringstream ss(content);
    std::string firstLine;
    if (std::getline(ss, firstLine)) {
        if (!firstLine.empty() && firstLine.back() == '\r') {
            firstLine.pop_back();
        }
        infoOutput = "SSDP " + firstLine;
    } else {
        infoOutput = "SSDP Data";
    }
    return true;
}

bool ApplicationParser::parse(ApplicationLayer& app, const uint8_t* data, size_t len,
                              uint16_t src_port, uint16_t dest_port, bool is_tcp)
{
    // --- Quyết định dựa trên cổng (Port) ---
    if ((src_port == 1900 || dest_port == 1900) && !is_tcp) {
        std::string ssdpInfo;
        if (parseSSDP(data, len, ssdpInfo)) {
            app.protocol = "SSDP";
            app.info = ssdpInfo;
            return true;
        }
    }
    // 1. Kiểm tra HTTP (cổng 80)
    if (src_port == 80 || dest_port == 80) {
        if (is_tcp && len > 0) {
            return HTTPParser::parse(app, data, len);
        }
    }

    // 2. Kiểm tra DNS (cổng 53)
    if (src_port == 53 || dest_port == 53) {
        if (!is_tcp && len > 0) {
            return DNSParser::parse(app, data, len);
        }
    }
    if (src_port == 5353 || dest_port == 5353) {
        if (!is_tcp) {
            app.protocol = "MDNS"; // <--- QUAN TRỌNG: Gán tên TRƯỚC khi parse

            if (DNSParser::parse(app, data, len)) {
                return true; // Parse thành công, giữ nguyên protocol MDNS
            }

            app.protocol = ""; // Nếu parse thất bại thì reset lại
        }
    }

    // 3. Kiểm tra Cổng 443 (TLS và QUIC)
    if (src_port == 443 || dest_port == 443) {

        // --- TRƯỜNG HỢP A: LÀ TCP ---
        if (is_tcp) {
            // Chỉ gán nhãn "TLS" nếu nó là TCP VÀ có mang dữ liệu (payload).
            if (len > 0) {
            app.protocol = "TLS"; // (TCP/443 là TLS, ra quyết định ngay)
                app.info = "Transport Layer Security";
                return true; // Đánh dấu là đã xử lý
            }
        }
        // --- TRƯỜNG HỢP B: LÀ UDP ---
        else { // (!is_tcp có nghĩa là UDP)

            if (len > 0) {

                // 1. Bit đầu tiên là 1 (Long Header)
                if ((data[0] & 0x80) != 0) {

                    // (Kiểm tra gói Version Negotiation y hệt Wireshark)
                    // Cần ít nhất 5 byte (1 byte header + 4 byte version)
                    if (len >= 5) {
                        uint32_t version;
                        // Đọc 4 byte version (bắt đầu từ byte thứ 2)
                        memcpy(&version, data + 1, 4);

                        // Nếu Version là 0x00000000, đây là Version Negotiation
                        if (version == 0) {
                            // Wireshark gọi đây là UDP. Chúng ta return false
                            // để nó tự động gán nhãn UDP.
                            return false;
                        }
                    }
                    app.quic_type = ApplicationLayer::QUIC_LONG_HEADER;

                    // (Phân tích sâu để lấy Info)
                    const uint8_t* ptr = data;
                    size_t remaining = len;
                    ptr++; remaining--;
                    if (remaining < 4) return true;
                    ptr += 4; remaining -= 4; // Bỏ qua 4 byte Version
                    if (remaining < 1) return true;
                    uint8_t dcid_len = ptr[0];
                    ptr++; remaining--;
                    if (remaining < dcid_len) return true;
                    std::string dcid_str = hexStr(ptr, dcid_len);
                    ptr += dcid_len; remaining -= dcid_len;
                    if (remaining < 1) return true;
                    uint8_t scid_len = ptr[0];
                    ptr++; remaining--;
                    if (remaining < scid_len) return true;
                    std::string scid_str = hexStr(ptr, scid_len);

                    uint8_t type_bits = (data[0] & 0x30) >> 4;
                    switch (type_bits) {
                    case 0x00: app.info = "Initial"; break;
                    case 0x01: app.info = "0-RTT"; break;
                    case 0x02: app.info = "Handshake"; break;
                    case 0x03: app.info = "Retry"; break;
                    default: app.info = "QUIC Long Header";
                    }
                    if (!dcid_str.empty()) {
                        app.info += ", DCID=" + dcid_str;
                    }
                    if (!scid_str.empty()) {
                        app.info += ", SCID=" + scid_str;
                    }

                    return true;
                }
                // 2. Bit đầu 0, bit hai 1 (Short Header)
                else if ((data[0] & 0xC0) == 0x40) {
                    app.quic_type = ApplicationLayer::QUIC_SHORT_HEADER;
                    return true; // Đánh dấu là đã xử lý
                }
            }
        } // Kết thúc 'else' (là UDP)
    }

    // Không tìm thấy parser nào
    return false;
}

void ApplicationParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    if (app.protocol == "HTTP") {
        HTTPParser::appendTreeView(tree, depth, app);
    }
    else if (app.protocol == "DNS") {
        DNSParser::appendTreeView(tree, depth, app);
    }
}
