#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QVector>
#include <QPair>
#include <QList> // <-- THÊM MỚI
#include "../Common/PacketData.hpp"
#include <QMessageBox>

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

public slots:
    // --- CÁC SLOT CÔNG KHAI (để AppController kết nối) ---

    /**
     * @brief Slot này nhận MỘT gói tin (ví dụ: khi bắt live)
     */
    void addPacketToTable(const PacketData &packet);

    /**
     * @brief (THÊM MỚI) Slot này nhận MỘT LÔ gói tin
     * (ví dụ: khi lọc lại) để tăng hiệu suất.
     */
    void addPacketsToTable(const QList<PacketData> &packets);

    /**
     * @brief Slot này yêu cầu PacketTable tự xóa sạch.
     */
    void clearPacketTable();

    /**
     * @brief Slot này nhận tín hiệu lỗi cú pháp
     */
    void showFilterError(const QString &errorText);

private slots:
    // --- Các slot xử lý cục bộ (cho các nút trên Header) ---
    void onMinimizeRequested();
    void onMaximizeRequested();
    void onCloseRequested();

signals:
    // === CÁC TÍN HIỆU (để AppController "lắng nghe") ===
    void interfaceSelected(const QString &interfaceName, const QString &filterText);
    void openFileRequested();
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);

private:
    HeaderWidget *header;
    QStackedWidget *stack;
    WelcomePage *welcomePage;
    CapturePage *capturePage;
};
