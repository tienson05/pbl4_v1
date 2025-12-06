#include "HeaderWidget.hpp"
#include "FileMenu.hpp"
#include "CaptureMenu.hpp"
#include "AnalyzeMenu.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

HeaderWidget::HeaderWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setupTitleBar(this, mainLayout);
    setupMenuBar(this, mainLayout);
}

void HeaderWidget::setupTitleBar(QWidget *parent, QVBoxLayout *mainLayout)
{
    QFrame *titleBar = new QFrame(parent);
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet("background-color: #2C3E50; color: white;");

    QLabel *titleLabel = new QLabel("WiresharkMini", titleBar);
    QPushButton *minBtn = new QPushButton("-", titleBar);
    QPushButton *maxBtn = new QPushButton("□", titleBar);
    QPushButton *closeBtn = new QPushButton("x", titleBar);

    QHBoxLayout *layout = new QHBoxLayout(titleBar);
    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(minBtn);
    layout->addWidget(maxBtn);
    layout->addWidget(closeBtn);

    connect(minBtn, &QPushButton::clicked, this, &HeaderWidget::minimizeRequested);
    connect(maxBtn, &QPushButton::clicked, this, &HeaderWidget::maximizeRequested);
    connect(closeBtn, &QPushButton::clicked, this, &HeaderWidget::closeRequested);

    mainLayout->addWidget(titleBar);
}

void HeaderWidget::setupMenuBar(QWidget *parent, QVBoxLayout *mainLayout)
{
    auto *menuBar = new QFrame(parent);
    QHBoxLayout *menuLayout = new QHBoxLayout(menuBar);
    menuLayout->setContentsMargins(10, 0, 0, 0);

    auto *fileMenu = new FileMenu(menuBar);
    auto *captureMenu = new CaptureMenu(menuBar);
    auto *analyzeMenu = new AnalyzeMenu(menuBar);

    // --- File Menu Connections ---
    connect(fileMenu, &FileMenu::openFileRequested, this, &HeaderWidget::openFileRequested);
    connect(fileMenu, &FileMenu::saveFileRequested, this, &HeaderWidget::saveFileRequested);
    connect(fileMenu, &FileMenu::closeRequested, this, &HeaderWidget::closeRequested);

    // --- Capture Menu Connections ---
    connect(captureMenu, &CaptureMenu::captureStartRequested, this, &HeaderWidget::captureStartRequested);

    // --- Analyze Menu Connections ---
    connect(analyzeMenu, &AnalyzeMenu::analyzeFlowRequested, this, &HeaderWidget::analyzeFlowRequested);
    connect(analyzeMenu, &AnalyzeMenu::analyzeStatisticsRequested, this, &HeaderWidget::analyzeStatisticsRequested);

    // [QUAN TRỌNG] THÊM DÒNG NÀY ĐỂ KẾT NỐI I/O GRAPH
    connect(analyzeMenu, &AnalyzeMenu::analyzeIOGraphRequested, this, &HeaderWidget::analyzeIOGraphRequested);

    menuLayout->addWidget(fileMenu);
    menuLayout->addWidget(captureMenu);
    menuLayout->addWidget(analyzeMenu);
    menuLayout->addStretch();

    mainLayout->addWidget(menuBar);
}
