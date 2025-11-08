#pragma once
#include <QPushButton>
#include <QMenu>
#include <QAction>

class AnalyzeMenu : public QPushButton
{
    Q_OBJECT
public:
    explicit AnalyzeMenu(QWidget *parent = nullptr);

signals:
    void analyzeFlowRequested();
    void analyzeStatisticsRequested();
};
