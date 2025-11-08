#pragma once
#include <QPushButton>
#include <QMenu>
#include <QAction>

class FileMenu : public QPushButton
{
    Q_OBJECT
public:
    explicit FileMenu(QWidget *parent = nullptr);

signals:
    void openFileRequested();
    void saveFileRequested();
    void closeRequested();
};
