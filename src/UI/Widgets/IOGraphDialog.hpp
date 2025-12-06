#ifndef IOGRAPHDIALOG_HPP
#define IOGRAPHDIALOG_HPP

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QComboBox>
#include <QPushButton>
#include "../../Controller/StatisticsManager.hpp"
#include "../../Common/PacketData.hpp"

// Dùng namespace của Qt Charts
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QT_USE_NAMESPACE
#else
using namespace QtCharts;
#endif

    class IOGraphDialog : public QDialog
{
    Q_OBJECT
public:
    // Nhận danh sách packet vào để xử lý
    explicit IOGraphDialog(const QList<PacketData>& packets, StatisticsManager* statsManager, QWidget *parent = nullptr);

public slots:
    void appendPackets(const QList<PacketData>& newPackets);
    void updateGraph();

private:
    void setupUi();

    // Dữ liệu
    QList<PacketData> m_packets;
    StatisticsManager* m_statsManager;

    // UI Components
    QChartView *m_chartView;
    QLineSeries *m_series;
    QChart *m_chart;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QComboBox *m_comboInterval;
    QComboBox *m_comboUnit;
    QPushButton *m_btnClose;
};

#endif // IOGRAPHDIALOG_HPP
