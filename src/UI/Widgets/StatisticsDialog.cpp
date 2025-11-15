#include "StatisticsDialog.hpp"
#include "../../Controller/StatisticsManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout> // <-- THÊM MỚI
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QLabel> // <-- THÊM MỚI

StatisticsDialog::StatisticsDialog(StatisticsManager* manager, QWidget *parent)
    : QDialog(parent),
    m_manager(manager)
{
    setupUi();
    setWindowTitle("Packet Statistics (Live)");
    resize(500, 400);
    setAttribute(Qt::WA_DeleteOnClose);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000); // 1 giây
    connect(m_updateTimer, &QTimer::timeout, this, &StatisticsDialog::onUpdateTimerTimeout);
}

void StatisticsDialog::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    // --- 1. (MỚI) TẠO CÁC LABEL TIÊU ĐỀ ---
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_totalPacketsLabel = new QLabel("Total Packets: 0", this);
    m_totalTypesLabel = new QLabel("Total Protocol Types: 0", this);

    // (Tùy chọn) Thêm style cho đậm
    m_totalPacketsLabel->setStyleSheet("font-weight: bold;");
    m_totalTypesLabel->setStyleSheet("font-weight: bold;");

    headerLayout->addWidget(m_totalPacketsLabel);
    headerLayout->addWidget(m_totalTypesLabel);
    headerLayout->addStretch(); // Đẩy về bên trái

    layout->addLayout(headerLayout); // Thêm các label vào layout chính
    // ------------------------------------

    // --- 2. TẠO THANH TAB ---
    m_tabWidget = new QTabWidget(this);
    m_protocolTree = createTreeWidget({"Protocol", "Packets", "Percent"});
    m_sourceTree = createTreeWidget({"Source IPs", "Packet Count"});
    m_destTree = createTreeWidget({"Destination IPs", "Packet Count"});
    m_tabWidget->addTab(m_protocolTree, "Protocols");
    m_tabWidget->addTab(m_sourceTree, "Sources");
    m_tabWidget->addTab(m_destTree, "Destinations");

    layout->addWidget(m_tabWidget); // Thêm tab widget vào layout chính
    setLayout(layout);
}

QTreeWidget* StatisticsDialog::createTreeWidget(const QStringList& headers)
{
    QTreeWidget* tree = new QTreeWidget();
    tree->setHeaderLabels(headers);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->sortByColumn(1, Qt::DescendingOrder);
    return tree;
}

void StatisticsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    onUpdateTimerTimeout();
    m_updateTimer->start();
}

void StatisticsDialog::closeEvent(QCloseEvent *event)
{
    m_updateTimer->stop();
    QDialog::closeEvent(event);
}

void StatisticsDialog::onUpdateTimerTimeout()
{
    // 1. Lấy dữ liệu MỚI NHẤT
    QMap<QString, qint64> protocols = m_manager->getProtocolCounts();
    QMap<QString, qint64> sources = m_manager->getSourceIpCounts();
    QMap<QString, qint64> dests = m_manager->getDestIpCounts();
    qint64 totalPackets = m_manager->getTotalPackets(); // <-- Lấy tổng số

    // --- 2. (MỚI) CẬP NHẬT LABEL TIÊU ĐỀ ---
    m_totalPacketsLabel->setText("Total Packets: " + QString::number(totalPackets));
    m_totalTypesLabel->setText("Total Protocol Types: " + QString::number(protocols.size()));
    // ------------------------------------

    // 3. Điền dữ liệu vào 3 tab (truyền totalPackets vào)
    populateTree(m_protocolTree, protocols, true, totalPackets);
    populateTree(m_sourceTree, sources, false, 0); // (Không cần totalPackets)
    populateTree(m_destTree, dests, false, 0);  // (Không cần totalPackets)
}

/**
 * @brief (ĐÃ CẬP NHẬT) Chấp nhận 'totalPackets'
 */
void StatisticsDialog::populateTree(QTreeWidget* tree,
                                    const QMap<QString, qint64>& data,
                                    bool showPercent,
                                    qint64 totalPackets) // <-- NHẬN TỔNG SỐ
{
    int sortColumn = tree->sortColumn();
    Qt::SortOrder sortOrder = tree->header()->sortIndicatorOrder();

    tree->clear();

    // (KHÔNG CẦN TÍNH LẠI TỔNG SỐ Ở ĐÂY NỮA)
    if (totalPackets == 0 && showPercent) totalPackets = 1; // Tránh chia cho 0

    QList<QTreeWidgetItem*> items;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
    {
        QString key = it.key();
        qint64 count = it.value();

        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, key);
        item->setText(1, QString::number(count));

        if (showPercent) {
            double percent = (static_cast<double>(count) / totalPackets) * 100.0;
            item->setText(2, QString::number(percent, 'f', 2) + "%");
        }
        items.append(item);
    }

    tree->addTopLevelItems(items);
    tree->sortByColumn(sortColumn, sortOrder);
}
