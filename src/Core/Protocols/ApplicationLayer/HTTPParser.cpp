#include "HTTPParser.hpp"
#include <cstring>
#include <string>
#include <sstream> // Cần cho string streaming
#include <vector>

// --- Hàm trợ giúp nội bộ ---

static void appendTree(std::string& tree, int depth, const std::string& line) {
    tree += std::string(depth * 2, ' ') + line + "\n";
}

// Tách một dòng (ví dụ: "GET /path HTTP/1.1")
static std::vector<std::string> split_line(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }
    return parts;
}

// --- Triển khai (Implementation) ---

bool HTTPParser::parse(ApplicationLayer& app, const uint8_t* data, size_t len) {
    if (len < 10) return false; // Quá nhỏ để là HTTP

    // Chuyển payload sang std::string để xử lý
    std::string payload_str(reinterpret_cast<const char*>(data), len);

    // Tìm dòng đầu tiên (kết thúc bằng \r\n)
    size_t first_line_end = payload_str.find("\r\n");
    if (first_line_end == std::string::npos) {
        return false; // Không tìm thấy dòng HTTP
    }
    std::string first_line = payload_str.substr(0, first_line_end);

    std::vector<std::string> parts = split_line(first_line);
    if (parts.empty()) return false;

    // --- Phân tích Yêu cầu (Request) ---
    // (GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH)
    if (parts[0] == "GET" || parts[0] == "POST" || parts[0] == "PUT" ||
        parts[0] == "DELETE" || parts[0] == "HEAD" || parts[0] == "OPTIONS" ||
        parts[0] == "PATCH")
    {
        if (parts.size() < 3) return false; // Phải là [METHOD] [PATH] [VERSION]

        app.is_http_request = true;
        app.protocol = "HTTP";
        app.http_method = parts[0];
        app.http_path = parts[1];
        app.http_version = parts[2];

        // Tìm Host header
        size_t host_start = payload_str.find("Host: ");
        if (host_start != std::string::npos) {
            size_t host_end = payload_str.find("\r\n", host_start);
            if (host_end != std::string::npos) {
                app.http_host = payload_str.substr(host_start + 6, host_end - (host_start + 6));
            }
        }

        app.info = app.http_method + " " + (app.http_host.empty() ? "" : app.http_host) + app.http_path;
        return true;
    }

    // --- Phân tích Phản hồi (Response) ---
    // (ví dụ: "HTTP/1.1 200 OK")
    if (parts[0].rfind("HTTP/", 0) == 0) // Bắt đầu bằng "HTTP/"
    {
        if (parts.size() < 2) return false; // Phải là [VERSION] [CODE] [STATUS]

        app.is_http_response = true;
        app.protocol = "HTTP";
        app.http_version = parts[0];
        try {
            app.http_status_code = std::stoi(parts[1]);
        } catch (...) {
            app.http_status_code = 0; // Lỗi
        }

        // Ghép phần còn lại của status (ví dụ: "OK", "Not Found")
        std::string status_message;
        for(size_t i = 2; i < parts.size(); ++i) {
            status_message += parts[i] + " ";
        }

        app.info = "Response " + std::to_string(app.http_status_code) + " " + status_message;
        return true;
    }

    return false; // Không phải HTTP
}

void HTTPParser::appendTreeView(std::string& tree, int depth, const ApplicationLayer& app) {
    appendTree(tree, depth, "Hypertext Transfer Protocol");
    depth++;

    if (app.is_http_request) {
        std::string req_line = app.http_method + " " + app.http_path + " " + app.http_version;
        appendTree(tree, depth, req_line);
        if (!app.http_host.empty()) {
            appendTree(tree, depth, "Host: " + app.http_host);
        }
        // (Bạn có thể thêm code để phân tích các header khác ở đây)
    }
    else if (app.is_http_response) {
        std::string status_line = app.http_version + " " + std::to_string(app.http_status_code);
        // (Code để lấy status message nếu bạn lưu nó)
        appendTree(tree, depth, status_line);
        // (Bạn có thể thêm code để phân tích các header khác ở đây)
    }
}
