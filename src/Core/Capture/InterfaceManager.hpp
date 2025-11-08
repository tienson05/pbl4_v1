#pragma once
#include <QString>
#include <QPair>
#include <QVector>

class InterfaceManager {
public:
    static QVector<QPair<QString, QString>> getDevices();

};
