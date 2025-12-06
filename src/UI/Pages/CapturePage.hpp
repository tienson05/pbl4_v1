#pragma once
#include <QWidget>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include "../Widgets/PacketTable.hpp"

class CapturePage : public QWidget
{
    Q_OBJECT

public:
    explicit CapturePage(QWidget *parent = nullptr);
    PacketTable *packetTable;

    void setInterfaceName(const QString &name, const QString &filter = QString());

    // --- (MỚI) Hàm để MainWindow điền text vào thanh filter ---
    void setFilterText(const QString &text);

signals:
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    /**
     * @brief Tín hiệu này được phát ra
     * khi người dùng nhấn nút "Statistics".
     */
    void onStatisticsClicked();

private:
    void setupUI();

    // --- BIẾN THÀNH VIÊN ---
    QLabel *sourceNameLabel;
    QString currentCaptureSource;
    bool isLiveCapture;

    QPushButton *restartBtn;
    QPushButton *stopBtn;
    QPushButton *pauseBtn;
    QPushButton *statisticsBtn;
    bool isPaused;

    // --- Thanh Filter ---
    QLineEdit *filterLineEdit;
    QPushButton *applyFilterButton;
};
