#ifndef STATISTICSDIALOG_HPP
#define STATISTICSDIALOG_HPP

#include <QDialog>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QLabel>

class QTabWidget;
class QTreeWidget;
class StatisticsManager;

class StatisticsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StatisticsDialog(StatisticsManager* manager, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onUpdateTimerTimeout();

private:
    void setupUi();
    QTreeWidget* createTreeWidget(const QStringList& headers);

    void populateTree(QTreeWidget* tree,
                      const QMap<QString, qint64>& data,
                      bool showPercent,
                      qint64 totalPackets);

    // --- BIẾN UI ---
    QLabel* m_totalPacketsLabel;
    QLabel* m_totalTypesLabel;
    QTabWidget* m_tabWidget;
    QTreeWidget* m_protocolTree;
    QTreeWidget* m_sourceTree;
    QTreeWidget* m_destTree;

    // --- BIẾN LOGIC ---
    StatisticsManager* m_manager;
    QTimer* m_updateTimer;
};

#endif // STATISTICSDIALOG_HPP
