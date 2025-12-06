#include "IOGraphDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

IOGraphDialog::IOGraphDialog(const QList<PacketData>& packets, StatisticsManager* statsManager, QWidget *parent)
    : QDialog(parent), m_packets(packets), m_statsManager(statsManager)
{
    setWindowTitle("I/O Graph - Traffic Analysis");
    resize(900, 600);
    setAttribute(Qt::WA_DeleteOnClose); // Tự xóa khi đóng để giải phóng RAM

    setupUi();
    updateGraph(); // Vẽ lần đầu
}

void IOGraphDialog::setupUi()
{
    // 1. Setup Chart
    m_series = new QLineSeries();
    m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->setTitle("Network Traffic");
    m_chart->legend()->hide();

    m_axisX = new QValueAxis();
    m_axisX->setTitleText("Time (s)");
    m_axisX->setLabelFormat("%.1f");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 2. Setup Controls (Thanh điều khiển bên dưới)
    QHBoxLayout *controlLayout = new QHBoxLayout();

    m_comboInterval = new QComboBox();
    m_comboInterval->addItem("1 sec", 1000);
    m_comboInterval->addItem("0.1 sec", 100);
    m_comboInterval->addItem("10 sec", 10000);
    m_comboInterval->addItem("1 min", 60000);

    m_comboUnit = new QComboBox();
    m_comboUnit->addItem("Bytes/Tick", 1);
    m_comboUnit->addItem("Packets/Tick", 0);

    m_btnClose = new QPushButton("Close");
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    controlLayout->addWidget(new QLabel("Interval:"));
    controlLayout->addWidget(m_comboInterval);
    controlLayout->addWidget(new QLabel("Unit:"));
    controlLayout->addWidget(m_comboUnit);
    controlLayout->addStretch();
    controlLayout->addWidget(m_btnClose);

    // Connect Combobox changes
    connect(m_comboInterval, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &IOGraphDialog::updateGraph);
    connect(m_comboUnit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &IOGraphDialog::updateGraph);

    // 3. Main Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_chartView);
    mainLayout->addLayout(controlLayout);
    setLayout(mainLayout);
}

void IOGraphDialog::updateGraph()
{
    if (!m_statsManager) return;

    int interval = m_comboInterval->currentData().toInt();
    bool modeBytes = m_comboUnit->currentData().toInt() == 1;

    // Tính toán lại
    QVector<QPointF> data = m_statsManager->calculateIOGraphData(m_packets, interval, modeBytes);

    // Cập nhật biểu đồ
    m_series->replace(data);

    // Cập nhật tiêu đề trục Y
    m_axisY->setTitleText(modeBytes ? "Bytes" : "Packets");
    m_axisY->setLabelFormat("%.0f");

    // Rescale trục
    if (!data.isEmpty()) {
        m_axisX->setRange(data.first().x(), data.last().x());

        double maxY = 0;
        for(const auto& p : data) if(p.y() > maxY) maxY = p.y();
        if (maxY == 0) maxY = 10;
        m_axisY->setRange(0, maxY * 1.1); // Thêm 10% khoảng trống bên trên
    }
}
void IOGraphDialog::appendPackets(const QList<PacketData>& newPackets)
{
    if (newPackets.isEmpty()) return;

    // 1. Thêm gói mới vào danh sách đang lưu trữ
    m_packets.append(newPackets);

    // 2. Vẽ lại biểu đồ
    updateGraph();
}
