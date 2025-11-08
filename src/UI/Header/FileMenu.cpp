#include "FileMenu.hpp"
#include <QMenu>

FileMenu::FileMenu(QWidget *parent)
    : QPushButton("File", parent)
{
    setStyleSheet("background: transparent; border: none; padding: 0px 10px;");
    setFont(QFont("Segoe UI", 10));

    QMenu *menu = new QMenu(this);
    menu->setStyleSheet("QMenu { background-color: white; border: 1px solid #D5D8DC; } "
                        "QMenu::item:selected { background-color: #4A90E2; color: white; }");

    QAction *openAct = menu->addAction("Open...");
    QAction *saveAsAct = menu->addAction("Save As...");
    menu->addSeparator();
    QAction *exitAct = menu->addAction("Exit");

    setMenu(menu);

    connect(openAct, &QAction::triggered, this, &FileMenu::openFileRequested);
    connect(saveAsAct, &QAction::triggered, this, &FileMenu::saveFileRequested);
    connect(exitAct, &QAction::triggered, this, &FileMenu::closeRequested);
}
