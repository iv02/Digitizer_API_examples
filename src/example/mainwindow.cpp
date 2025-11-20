#include "mainwindow.h"
#include "devicecontrolpanel.h"
#include "settingspanel.h"
#include "waveformspectrumwidget.h"
#include "datatablewidget.h"
#include "digitizerinteractor.h"
#include "packetwrappers/eventdata.h"

#include <QGridLayout>
#include <QMetaObject>
#include <QTimer>

using namespace digi;

namespace
{
    struct EventDataMetaType
    {
        EventDataMetaType()
        {
            qRegisterMetaType<network::EventData>("network::EventData");
            qRegisterMetaType<QVector<network::EventData>>("QVector<network::EventData>");
        }
    };
    static EventDataMetaType eventDataMetaType;
}

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent)
    , m_digitizerInteractor(nullptr)
    , m_deviceControlPanel(nullptr)
    , m_settingsPanel(nullptr)
    , m_waveformSpectrumWidget(nullptr)
    , m_dataTableWidget(nullptr)
    , m_updateTimer(nullptr)
    , m_hasPendingData(false)
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
    m_waveformSpectrumWidget = new WaveformSpectrumWidget(m_digitizerInteractor, this);
    m_dataTableWidget = new DataTableWidget(m_digitizerInteractor, this);

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(m_deviceControlPanel, 0, 0);
    gridLayout->addWidget(m_settingsPanel, 1, 0);
    gridLayout->addWidget(m_waveformSpectrumWidget, 0, 1);
    gridLayout->addWidget(m_dataTableWidget, 1, 1);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setRowStretch(0, 1);
    gridLayout->setRowStretch(1, 1);

    auto centralWidget = new QWidget(this);
    centralWidget->setLayout(gridLayout);
    setCentralWidget(centralWidget);

    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(300);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateTimer);
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

    m_digitizerInteractor->setDataEventCallback([this](const network::EventData &eventData) {
        QMetaObject::invokeMethod(this, [this, eventData]() {
            m_pendingEventData = eventData;
            m_hasPendingData = true;

            if (!m_updateTimer->isActive())
            {
                m_updateTimer->start();
            }
        }, Qt::QueuedConnection);
    });

    m_digitizerInteractor->setDataBatchCallback([this](const QVector<network::EventData> &batch) {
        QMetaObject::invokeMethod(this, [this, batch]() {
            if (!batch.isEmpty())
            {
                m_pendingEventData = batch.last();
                m_hasPendingData = true;

                if (!m_updateTimer->isActive())
                {
                    m_updateTimer->start();
                }
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onUpdateTimer()
{
    if (m_hasPendingData)
    {
        QMetaObject::invokeMethod(m_waveformSpectrumWidget, "processEventData", Qt::QueuedConnection,
                                  Q_ARG(const network::EventData &, m_pendingEventData));
        QMetaObject::invokeMethod(m_dataTableWidget, "processEventData", Qt::QueuedConnection,
                                  Q_ARG(const network::EventData &, m_pendingEventData));
        m_hasPendingData = false;
    }
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
