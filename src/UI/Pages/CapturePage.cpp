#include "CapturePage.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>

CapturePage::CapturePage(QWidget *parent)
    : QWidget(parent),
    sourceNameLabel(new QLabel(this)),
    isLiveCapture(false),
    restartBtn(new QPushButton("Restart", this)),
    stopBtn(new QPushButton("Stop", this)),
    pauseBtn(new QPushButton("Pause", this)),
    statisticsBtn(new QPushButton("Statistics", this)),
    isPaused(false),
    filterLineEdit(new QLineEdit(this)),
    applyFilterButton(new QPushButton("Apply", this)),
    packetTable(new PacketTable(this))
{
    setupUI();

    // --- Kết nối signal-slot ---
    connect(restartBtn, &QPushButton::clicked, this, &CapturePage::onRestartCaptureClicked);
    connect(stopBtn, &QPushButton::clicked, this, &CapturePage::onStopCaptureClicked);
    connect(pauseBtn, &QPushButton::clicked, this, &CapturePage::onPauseCaptureClicked);
    connect(applyFilterButton, &QPushButton::clicked, this, [=](){
        emit onApplyFilterClicked(filterLineEdit->text());
    });
    connect(statisticsBtn, &QPushButton::clicked, this, &CapturePage::onStatisticsClicked);

    // (MỚI) Cho phép nhấn Enter ở thanh Filter để Apply luôn
    connect(filterLineEdit, &QLineEdit::returnPressed, applyFilterButton, &QPushButton::click);
}

void CapturePage::setupUI()
{
    // --- Cấu hình nhãn nguồn ---
    sourceNameLabel->setText("No source selected");
    sourceNameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

    // --- Hàng nút điều khiển ---
    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(sourceNameLabel);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(restartBtn);
    controlLayout->addWidget(stopBtn);
    controlLayout->addWidget(pauseBtn);
    controlLayout->addWidget(statisticsBtn);
    controlLayout->addStretch();

    // --- Thanh filter ---
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLineEdit->setPlaceholderText("Enter display filter (e.g., tcp, udp, stream == 1)");
    filterLayout->addWidget(filterLineEdit);
    filterLayout->addWidget(applyFilterButton);

    // --- Layout phía trên: điều khiển + filter ---
    QVBoxLayout *topLayout = new QVBoxLayout;
    topLayout->addLayout(controlLayout);
    topLayout->addLayout(filterLayout);

    // --- Layout chính ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(packetTable, 1); // Bảng chiếm phần còn lại của cửa sổ
    setLayout(mainLayout);
    setStyleSheet(R"(
        QPushButton {
            padding: 5px 12px;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
        }
        QLineEdit {
            padding: 4px;
        }
    )");
}

void CapturePage::setInterfaceName(const QString &name, const QString &filter)
{
    // Tạo chuỗi cơ bản
    QString displayText = "Interface: " + name;

    // Nếu có filter thì thêm vào
    if (!filter.isEmpty()) {
        displayText += " (" + filter + ")";
    }

    // Cập nhật lên màn hình
    if (sourceNameLabel) {
        sourceNameLabel->setText(displayText);
    }
}

// --- (MỚI) Triển khai hàm setFilterText ---
void CapturePage::setFilterText(const QString &text)
{
    if (filterLineEdit) {
        filterLineEdit->setText(text);
    }
}
