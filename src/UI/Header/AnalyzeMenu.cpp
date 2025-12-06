#include "AnalyzeMenu.hpp"

AnalyzeMenu::AnalyzeMenu(QWidget *parent)
    : QPushButton("Analyze", parent)
{
    setStyleSheet("background: transparent; border: none; padding: 0px 10px;");
    setFont(QFont("Segoe UI", 10));

    QMenu *menu = new QMenu(this);
    menu->setStyleSheet("QMenu { background-color: white; border: 1px solid #D5D8DC; } "
                        "QMenu::item:selected { background-color: #4A90E2; color: white; }");

    QAction *flowAct = menu->addAction("Packet Flow");
    QAction *statsAct = menu->addAction("Statistics");
QAction *ioGraphAct = menu->addAction("I/O Graph");
    setMenu(menu);

    connect(flowAct, &QAction::triggered, this, &AnalyzeMenu::analyzeFlowRequested);
    connect(statsAct, &QAction::triggered, this, &AnalyzeMenu::analyzeStatisticsRequested);
connect(ioGraphAct, &QAction::triggered, this, &AnalyzeMenu::analyzeIOGraphRequested);
}
