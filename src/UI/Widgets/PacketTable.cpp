// PacketTable.cpp
#include "PacketTable.hpp"
#include "PacketFormatter.hpp"
#include <QVBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QTimer>

const int CHUNK_SIZE = 500;
const int TIMER_INTERVAL_MS = 30;

PacketTable::PacketTable(QWidget *parent) : QWidget(parent)
{
    setupUI();

    // Setup Context Menu
    packetList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(packetList, &QTableWidget::customContextMenuRequested, this, &PacketTable::showContextMenu);

    // Click Events
    connect(packetList, &QTableWidget::itemClicked, this, &PacketTable::onPacketRowSelected);
    connect(packetDetails, &QTreeWidget::itemClicked, this, &PacketTable::onDetailRowSelected);

    // Timer Update
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(TIMER_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &PacketTable::processPacketChunk);
    m_updateTimer->start();

    // Auto Scroll Logic
    connect(packetList->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value){
        QScrollBar* vbar = packetList->verticalScrollBar();
        m_isUserAtBottom = (vbar == nullptr) || (value >= vbar->maximum() - 5);
    });
}

void PacketTable::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    packetList = new QTableWidget(this);

    packetList->setColumnCount(8);
    QStringList headers = {"No.", "Time", "Source", "Destination", "Protocol", "Length", "Info"};
    packetList->setHorizontalHeaderLabels(headers);
    packetList->horizontalHeader()->setStretchLastSection(true);
    packetList->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packetList->hideColumn(7); // Cột ẩn chứa dữ liệu
    packetList->verticalHeader()->setVisible(false);

    packetDetails = new QTreeWidget(this);
    packetDetails->setHeaderLabel("Packet Details");
    packetDetails->setColumnCount(1);

    packetBytes = new QTextEdit(this);
    packetBytes->setReadOnly(true);
    packetBytes->setAcceptRichText(true); // Để hiển thị màu HTML

    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, this);
    bottomSplitter->addWidget(packetDetails);
    bottomSplitter->addWidget(packetBytes);
    bottomSplitter->setSizes({400, 300});

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(packetList);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setSizes({300, 400});

    layout->addWidget(mainSplitter);
    setLayout(layout);
}

void PacketTable::clearData()
{
    packetList->setRowCount(0);
    packetDetails->clear();
    packetBytes->clear();
    m_packetBuffer.clear();
    m_allPackets.clear();
}

void PacketTable::applyFilter(const QString& filterText)
{
    m_currentFilter = filterText;
    refreshTable();
}

void PacketTable::refreshTable()
{
    packetList->setUpdatesEnabled(false);
    packetList->setSortingEnabled(false);
    packetList->setRowCount(0);

    for (const auto& packet : m_allPackets) {
        if (m_filterEngine.match(packet, m_currentFilter)) {
            insertPacketRow(packet);
        }
    }

    packetList->setSortingEnabled(true);
    packetList->setUpdatesEnabled(true);
    if (m_isUserAtBottom) packetList->scrollToBottom();
}

void PacketTable::onPacketReceived(const PacketData &packet)
{
    m_allPackets.append(packet);
    m_packetBuffer.append(packet);
}

void PacketTable::onPacketsReceived(QList<PacketData>* packets)
{
    m_allPackets.append(*packets);
    m_packetBuffer.append(std::move(*packets));
    delete packets;
}

void PacketTable::processPacketChunk()
{
    if (m_packetBuffer.isEmpty()) return;

    packetList->setUpdatesEnabled(false);
    packetList->setSortingEnabled(false);

    int packetsToProcess = qMin(CHUNK_SIZE, (int)m_packetBuffer.size());
    for (int i = 0; i < packetsToProcess; ++i) {
        insertPacketRow(m_packetBuffer.takeFirst());
    }

    packetList->setSortingEnabled(true);
    packetList->setUpdatesEnabled(true);

    if (m_isUserAtBottom) packetList->scrollToBottom();
}

void PacketTable::insertPacketRow(const PacketData &packet)
{
    int row = packetList->rowCount();
    packetList->insertRow(row);

    QString proto = PacketFormatter::getProtocolName(packet);

    QList<QTableWidgetItem*> items = {
        new QTableWidgetItem(QString::number(packet.packet_id)),
        new QTableWidgetItem(PacketFormatter::formatTime(packet.timestamp)),
        new QTableWidgetItem(PacketFormatter::getSource(packet)),
        new QTableWidgetItem(PacketFormatter::getDest(packet)),
        new QTableWidgetItem(proto),
        new QTableWidgetItem(QString::number(packet.wire_length)),
        new QTableWidgetItem(PacketFormatter::getInfo(packet))
    };

    QColor color = PacketFormatter::getRowColor(proto);

    for(int i = 0; i < items.size(); ++i) {
        items[i]->setBackground(color);
        packetList->setItem(row, i, items[i]);
    }

    QTableWidgetItem* hidden = new QTableWidgetItem();
    hidden->setData(Qt::UserRole, QVariant::fromValue(packet));
    hidden->setFlags(Qt::NoItemFlags);
    packetList->setItem(row, 7, hidden);
}

void PacketTable::onPacketRowSelected(QTableWidgetItem *item)
{
    if (!item) return;
    QTableWidgetItem *hidden = packetList->item(item->row(), 7);
    if (!hidden) return;

    m_currentSelectedPacket = hidden->data(Qt::UserRole).value<PacketData>();

    PacketFormatter::populateTree(packetDetails, m_currentSelectedPacket);
    PacketFormatter::displayHexDump(packetBytes, m_currentSelectedPacket);
}

void PacketTable::onDetailRowSelected(QTreeWidgetItem *item, int column)
{
    if (!item) return;
    QVariant offsetData = item->data(0, Qt::UserRole + 1);
    QVariant lengthData = item->data(0, Qt::UserRole + 2);

    if (offsetData.isValid() && lengthData.isValid()) {
        PacketFormatter::displayHexDump(packetBytes, m_currentSelectedPacket, offsetData.toInt(), lengthData.toInt());
    } else {
        PacketFormatter::displayHexDump(packetBytes, m_currentSelectedPacket);
    }
}

void PacketTable::showContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = packetList->itemAt(pos);
    if (!item) return;

    QTableWidgetItem *hidden = packetList->item(item->row(), 7);
    PacketData packet = hidden->data(Qt::UserRole).value<PacketData>();
    if (packet.stream_index < 0) return;

    QMenu contextMenu(this);
    QString streamName = (packet.app.protocol == "QUIC") ? "QUIC" : "TCP/UDP";
    QAction *actionFollow = contextMenu.addAction(QString("Follow %1 Stream (#%2)").arg(streamName).arg(packet.stream_index));

    connect(actionFollow, &QAction::triggered, [this, packet]() {
        emit filterRequested(QString("stream == %1").arg(packet.stream_index));
    });

    contextMenu.exec(packetList->viewport()->mapToGlobal(pos));
}
