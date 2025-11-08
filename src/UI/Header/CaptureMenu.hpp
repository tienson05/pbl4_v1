#pragma once
#include <QPushButton>
#include <QMenu>
#include <QAction>

class CaptureMenu : public QPushButton
{
    Q_OBJECT
public:
    explicit CaptureMenu(QWidget *parent = nullptr);

signals:
    void captureStartRequested();
};
