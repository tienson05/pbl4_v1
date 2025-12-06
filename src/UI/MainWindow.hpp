#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QVector>
#include <QPair>
#include "../Common/PacketData.hpp"
#include <QMessageBox>
#include "Header/AnalyzeMenu.hpp"

// Khai báo trước
class HeaderWidget;
class WelcomePage;
class CapturePage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // --- Các hàm tiện ích (do AppController gọi) ---
    void showWelcomePage();
    void showCapturePage();
    void setDevices(const QVector<QPair<QString, QString>> &devices);
void updateInterfaceLabel(const QString &name, const QString &filter);
public slots:
    // --- CÁC SLOT CÔNG KHAI (để AppController kết nối) ---

    /**
     * @brief (ĐÃ THAY ĐỔI) Slot nhận tín hiệu "lô" (batch) từ AppController
     * và chuyển tiếp "lô" đó xuống PacketTable.
     */
    void addPacketsToTable(QList<PacketData>* packets);

    /**
     * @brief (Giữ nguyên) Slot nhận tín hiệu từ AppController
     * và yêu cầu PacketTable tự xóa sạch.
     */
    void clearPacketTable();

    /**
     * @brief (Giữ nguyên) Slot này nhận tín hiệu lỗi cú pháp
     * từ AppController và hiển thị một QMessageBox.
     */
    void showFilterError(const QString &errorText);
    void applyStreamFilter(const QString &filterText);

private slots:
    // --- Các slot xử lý cục bộ (cho các nút trên Header) ---
    void onMinimizeRequested();
    void onMaximizeRequested();
    void onCloseRequested();

signals:
    // === CÁC TÍN HIỆU (để AppController "lắng nghe") ===
    // (Đây là các tín hiệu được "forward" (chuyển tiếp) từ các Page con)

    // Signals từ WelcomePage
    void interfaceSelected(const QString &interfaceName, const QString &filterText);
    void openFileRequested();

    // Signals từ CapturePage
    void saveFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText); // <-- Filter hiển thị
    void analyzeStatisticsRequested(); // <-- THÊM CÁI NÀY
    void analyzeIOGraphRequested(); // <-- THÊM MỚI
private:
    HeaderWidget *header;
    QStackedWidget *stack;
    WelcomePage *welcomePage;
    CapturePage *capturePage;
    AnalyzeMenu *m_analyzeMenu;
};
