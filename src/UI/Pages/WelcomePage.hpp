#pragma once
#include <QWidget>
#include <QListWidget>

class QLineEdit;

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);
    void setDevices(const QVector<QPair<QString, QString>> &devices);

signals:
    void interfaceSelected(const QString &interfaceName, const QString &filterText);
    void openFileRequested();

private:
    QLineEdit *filterEdit;
    QListWidget *deviceList;
    void setupUI();
};
