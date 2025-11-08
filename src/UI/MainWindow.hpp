#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "Pages/WelcomePage.hpp"
#include "Pages/CapturePage.hpp"
#include "Header/HeaderWidget.hpp"
#include "../Common/PacketData.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void showWelcomePage();
    void showCapturePage();
    void addPacketToTable(const PacketData &packet);
    void setDevices(const QVector<QPair<QString, QString>> &devices);

private:
    HeaderWidget *header;
    QStackedWidget *stack;
    WelcomePage *welcomePage;
    CapturePage *capturePage;

signals:
    // Signals từ WelcomePage
    void interfaceSelected(const QString &interfaceName, const QString &filterText);
    void openFileRequested();
    // Signals từ CapturePage
    void onRestartCaptureClicked();
    void onStopCaptureClicked();
    void onPauseCaptureClicked();
    void onApplyFilterClicked(const QString &filterText);
    // Signals từ Header
    // ...

// Signals ko liên quan đến Core thì xử lí đây luôn
private slots:
    void onMinimizeRequested();
    void onMaximizeRequested();
    void onCloseRequested();
};
