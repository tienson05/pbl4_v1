#ifndef MACRESOLVER_HPP
#define MACRESOLVER_HPP

#include <string>
#include <unordered_map>
#include <mutex>

class MacResolver {
public:
    // Singleton access
    static MacResolver& instance();

    // Load dữ liệu từ file manuf
    bool loadDatabase(const std::string& filePath);

    // Hàm lấy tên vendor từ MAC address (dạng chuỗi "00:11:22:33:44:55")
    std::string getVendor(const std::string& macAddress);

private:
    MacResolver() = default;
    ~MacResolver() = default;
    MacResolver(const MacResolver&) = delete;
    MacResolver& operator=(const MacResolver&) = delete;

    // Map lưu trữ: Key là 6 ký tự đầu (OUI), Value là tên Vendor
    std::unordered_map<std::string, std::string> ouiTable;

    std::string extractOUI(const std::string& mac);
};

#endif // MACRESOLVER_HPP
