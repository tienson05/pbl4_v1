#include "MacResolver.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

MacResolver& MacResolver::instance() {
    static MacResolver instance;
    return instance;
}

bool MacResolver::loadDatabase(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open manuf file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Bỏ qua dòng comment hoặc dòng trống
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string oui, shortName;

        // File manuf thường có dạng: 00:00:01  Xerox  # Xerox Corporation
        // Chúng ta chỉ cần lấy OUI và Short Name
        ss >> oui >> shortName;

        // Chuẩn hóa OUI về dạng string không có dấu : hoặc - để làm key
        // Ví dụ: 00:00:01 -> 000001
        oui.erase(std::remove(oui.begin(), oui.end(), ':'), oui.end());
        oui.erase(std::remove(oui.begin(), oui.end(), '-'), oui.end());

        // Chỉ lấy 6 ký tự đầu (chuẩn OUI 24-bit)
        if (oui.length() >= 6) {
            // Lưu vào map (chuyển về uppercase để đồng bộ)
            std::transform(oui.begin(), oui.end(), oui.begin(), ::toupper);
            ouiTable[oui.substr(0, 6)] = shortName;
        }
    }

    std::cout << "Loaded " << ouiTable.size() << " OUI records." << std::endl;
    return true;
}

std::string MacResolver::extractOUI(const std::string& mac) {
    std::string temp = mac;
    // Xóa ký tự phân cách
    temp.erase(std::remove(temp.begin(), temp.end(), ':'), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '-'), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '.'), temp.end());

    if (temp.length() < 6) return "";

    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp.substr(0, 6);
}

std::string MacResolver::getVendor(const std::string& macAddress) {
    std::string oui = extractOUI(macAddress);
    if (oui.empty()) return "Unknown";

    auto it = ouiTable.find(oui);
    if (it != ouiTable.end()) {
        return it->second;
    }
    return "Unknown";
}
