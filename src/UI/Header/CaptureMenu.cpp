#include "CaptureMenu.hpp"

CaptureMenu::CaptureMenu(QWidget *parent)
    : QPushButton("Capture", parent)
{
    setStyleSheet("background: transparent; border: none; padding: 0px 10px;");
    setFont(QFont("Segoe UI", 10));

    QMenu *menu = new QMenu(this);
    menu->setStyleSheet("QMenu { background-color: white; border: 1px solid #D5D8DC; } "
                        "QMenu::item:selected { background-color: #4A90E2; color: white; }");

    QAction *startAct = menu->addAction("Start");
    setMenu(menu);

    connect(startAct, &QAction::triggered, this, &CaptureMenu::captureStartRequested);
}
