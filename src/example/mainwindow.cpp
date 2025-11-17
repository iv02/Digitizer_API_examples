#include "mainwindow.h"
#include "devicecontrolpanel.h"
#include "settingspanel.h"
#include "digitizerinteractor.h"

#include <QSplitter>
#include <QVBoxLayout>

using namespace digi;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    setupConnections();
    setStyle();
}

MainWindow::~MainWindow()
{
    delete m_digitizerInteractor;
}

void MainWindow::setupUi()
{
    setWindowTitle("Test digitizer-wrapper");
    setMinimumSize(1000, 800);

    m_digitizerInteractor = new digi::DigitizerInteractor();
    m_deviceControlPanel = new DeviceControlPanel(m_digitizerInteractor, this);
    m_settingsPanel = new SettingsPanel(m_digitizerInteractor, this);

    auto splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_deviceControlPanel);
    splitter->addWidget(m_settingsPanel);

    auto centralWidget = new QWidget(this);
    centralWidget->setLayout(new QVBoxLayout());
    setCentralWidget(centralWidget);
    centralWidget->layout()->addWidget(splitter);
}

void MainWindow::setupConnections()
{
    connect(m_deviceControlPanel, &DeviceControlPanel::deviceSelectionChanged, this,
            &MainWindow::onDeviceSelectionChanged);
    connect(m_deviceControlPanel, &DeviceControlPanel::deviceConnected, this,
            [this](int64_t deviceId) {
                if (m_settingsPanel->hasActiveSettings())
                {
                    m_settingsPanel->refreshFwTypeButtons();
                }
            });
}

void MainWindow::onDeviceSelectionChanged(int64_t deviceId)
{
    if (deviceId >= 0)
    {
        if (!m_settingsPanel->hasActiveSettings())
        {
            m_settingsPanel->showSettings(deviceId, "Device");
        }
        else
        {
            m_settingsPanel->showSettings(deviceId, m_settingsPanel->currentSettingsType());
        }
    }
    else
    {
        m_settingsPanel->hideSettings();
    }
}

void MainWindow::setStyle()
{
    setStyleSheet("QTextEdit { line-height: 150%; font-family: Courier New; font-size: 12pt; background-color: #f0f0f0; } "
                  "QMenu::item { padding-left: 4px; padding-right: 4px; background: transparent; } "
                  "QMenu::item:selected { background-color: palette(base); color: palette(text); } "
                  "QTableView { "
                  "    background-color: #f0f0f0; "
                  "    font-family: Arial; "
                  "    font-size: 12pt; "
                  "    font-weight : normal; "
                  "    alternate-background-color: #f8f8f8; "
                  "    selection-background-color : #C4DDFF; "
                  "    selection-color: palette(text); "
                  "    gridline-color: #BFBFBF; "
                  "} "
                  "QTableView::item { "
                  "    border: none; "
                  "    margin-left: 5px; "
                  "    margin-right: 10px; "
                  "} "
                  "QTableView QHeaderView::section { "
                  "    font-family: Arial; "
                  "    font-weight : normal; "
                  "    font-size: 10pt; "
                  "    text-align: left; "
                  "    padding: 5px 10px; "
                  "    background: #f0f0f0; "
                  "    border: none; "
                  "    border-bottom: 1px solid #BFBFBF; "
                  "} ");
}
