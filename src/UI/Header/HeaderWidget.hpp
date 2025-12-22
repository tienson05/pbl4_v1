#pragma once
#include <QWidget>
#include <QVBoxLayout>

class HeaderWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeaderWidget(QWidget *parent = nullptr);
signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

    void openFileRequested();
    void saveFileRequested();
    void analyzeFlowRequested();
    void analyzeStatisticsRequested();
    void analyzeIOGraphRequested();

private:
    void setupTitleBar(QWidget *parent, QVBoxLayout *mainLayout);
    void setupMenuBar(QWidget *parent, QVBoxLayout *mainLayout);
};
