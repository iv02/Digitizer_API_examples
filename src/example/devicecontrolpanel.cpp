#include "devicecontrolpanel.h"
#include "digitizerinteractor.h"

#include <QHeaderView>
#include <QJsonDocument>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTimer>

using namespace digi;

DeviceControlPanel::DeviceControlPanel(DigitizerInteractor *interactor, QWidget *parent)
    : QWidget(parent), m_interactor(interactor)
{
    setupUi();
    setupConnections();
    setupTableStyle();
}

int64_t DeviceControlPanel::currentDeviceId() const
{
    int64_t id = -1;
    if (auto index = m_devicesTable->currentIndex(); index.isValid())
    {
        index = m_devicesTable->model()->index(index.row(), 1);
        if (auto idv = m_devicesTable->model()->data(index); idv.isValid())
        {
            bool ok;
            if (id = idv.toInt(&ok); !ok)
                id = -1;
        }
    }
    return id;
}

void DeviceControlPanel::refreshDevices()
{
    m_devices = m_interactor->devices();

    auto id = m_devicesModel->data(m_devicesModel->index(m_devicesTable->currentIndex().row(), 1));

    m_devicesModel->blockSignals(true);
    m_devicesModel->setRowCount(static_cast<int>(m_devices.size()));
    for (int row = 0; auto &items : m_devices)
    {
        for (int col = 0; col < items.size(); ++col)
            m_devicesModel->setData(m_devicesModel->index(row, col), items[col], Qt::EditRole);
        ++row;
    }
    m_devicesModel->blockSignals(false);
    m_devicesModel->layoutChanged();

    if (id.isValid())
    {
        for (int row = 0; row < m_devicesModel->rowCount(); ++row)
            if (auto index = m_devicesModel->index(row, 1); id == m_devicesModel->data(index))
            {
                m_devicesTable->selectRow(index.row());
                m_devicesTable->scrollTo(index);
            }
    }
    else if (m_devicesModel->rowCount() > 0)
    {
        m_devicesTable->selectRow(0);
        m_devicesTable->scrollTo(m_devicesTable->model()->index(0, 0));
    }

    onDeviceSelectionChanged();
}

void DeviceControlPanel::appendLog(const QString &text)
{
    logMessage(text);
}

void DeviceControlPanel::logMessage(const QString &message)
{
    m_logEdit->append(message);
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
}

void DeviceControlPanel::clearLog()
{
    m_logEdit->clear();
}

void DeviceControlPanel::setupUi()
{
    m_devicesModel = new QStandardItemModel(this);
    // Get device table column headers from DigitizerInteractor
    auto headerLabels = m_interactor->deviceHeaderLabels();
    m_devicesModel->setColumnCount(static_cast<int>(headerLabels.size()));
    m_devicesModel->setHorizontalHeaderLabels(headerLabels);

    m_devicesTable = new QTableView(this);
    m_devicesTable->setModel(m_devicesModel);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setLineWrapMode(QTextEdit::NoWrap);

    auto splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_logEdit);
    splitter->addWidget(m_devicesTable);
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 2);

    m_menuBar = new QMenuBar(this);
    
    m_deviceMenu = m_menuBar->addMenu("Device");
    m_actionConnectDevice = m_deviceMenu->addAction("Connect device");
    m_actionDisconnectDevice = m_deviceMenu->addAction("Disconnect device");
    m_deviceMenu->addSeparator();
    m_actionStartMeasure = m_deviceMenu->addAction("Start measure");
    m_actionStopMeasure = m_deviceMenu->addAction("Stop measure");
    
    m_settingsMenu = m_menuBar->addMenu("Settings");
    m_actionShowFirmwareSettings = m_settingsMenu->addAction("Show FW Settings");
    m_actionUploadSettings = m_settingsMenu->addAction("Upload settings");
    m_actionDownloadSettings = m_settingsMenu->addAction("Download settings");
    
    m_logMenu = m_menuBar->addMenu("Log");
    m_actionClearLog = m_logMenu->addAction("Clear log");

    auto mainLayout = new QVBoxLayout();
    mainLayout->setMenuBar(m_menuBar);
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
}

void DeviceControlPanel::setupConnections()
{
    m_interactor->setDeviceDiscoveryCallback([this](int64_t deviceId) {
        QTimer::singleShot(0, this, [this, deviceId]() {
            onDeviceDiscovered(deviceId);
        });
    });

    connect(m_actionConnectDevice, &QAction::triggered, this, &DeviceControlPanel::onConnectDevice);
    connect(m_actionDisconnectDevice, &QAction::triggered, this, &DeviceControlPanel::onDisconnectDevice);
    connect(m_actionStartMeasure, &QAction::triggered, this, &DeviceControlPanel::onStartMeasure);
    connect(m_actionStopMeasure, &QAction::triggered, this, &DeviceControlPanel::onStopMeasure);
    connect(m_actionClearLog, &QAction::triggered, this, &DeviceControlPanel::onClearLog);
    connect(m_actionShowFirmwareSettings, &QAction::triggered, this, &DeviceControlPanel::onLogFirmwareSettings);
    connect(m_actionUploadSettings, &QAction::triggered, this, &DeviceControlPanel::onUploadSettings);
    connect(m_actionDownloadSettings, &QAction::triggered, this, &DeviceControlPanel::onDownloadSettings);
    connect(m_devicesTable->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &DeviceControlPanel::onDeviceSelectionChanged);
}

void DeviceControlPanel::setupTableStyle()
{
    m_devicesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_devicesTable->horizontalHeader()->setStretchLastSection(true);
    m_devicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_devicesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_devicesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_devicesTable->verticalHeader()->setVisible(false);
    m_devicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void DeviceControlPanel::onDeviceDiscovered(int64_t deviceId)
{
    m_devices = m_interactor->devices();

    auto id = m_devicesModel->data(m_devicesModel->index(m_devicesTable->currentIndex().row(), 1));

    m_devicesModel->blockSignals(true);
    m_devicesModel->setRowCount(static_cast<int>(m_devices.size()));
    for (int row = 0; auto &items : m_devices)
    {
        for (int col = 0; col < items.size(); ++col)
            m_devicesModel->setData(m_devicesModel->index(row, col), items[col], Qt::EditRole);
        ++row;
    }
    m_devicesModel->blockSignals(false);
    m_devicesModel->layoutChanged();

    if (id.isValid())
    {
        for (int row = 0; row < m_devicesModel->rowCount(); ++row)
            if (auto index = m_devicesModel->index(row, 1); id == m_devicesModel->data(index))
            {
                m_devicesTable->selectRow(index.row());
                m_devicesTable->scrollTo(index);
            }
    }
    else if (m_devicesModel->rowCount() > 0)
    {
        m_devicesTable->selectRow(0);
        m_devicesTable->scrollTo(m_devicesTable->model()->index(0, 0));
    }

    logMessage(QString("Device discovered: id = %1, total count = %2").arg(deviceId).arg(m_devices.size()));
    onDeviceSelectionChanged();
}

void DeviceControlPanel::onConnectDevice()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        return;
    }

    // Connect to the selected device using DigitizerInteractor
    auto r = m_interactor->connectDevice(id);
    // Get the number of channels for the connected device
    auto channels = QString("; Channels = ") + QString::number(m_interactor->getDeviceChannels(id));
    logMessage(QString("Id %1: Device %2 connected").arg(id).arg(r ? "" : "not") + channels);
    if (r)
    {
        emit deviceConnected(id);
    }
}

void DeviceControlPanel::onDisconnectDevice()
{
    auto id = currentDeviceId();
    if (id < 0)
        return;

    // Disconnect from the selected device using DigitizerInteractor
    auto r = m_interactor->disconnectDevice(id);
    logMessage(QString("Id %1: Device %2 disconnected").arg(id).arg(r ? "" : "not"));
}

void DeviceControlPanel::onStartMeasure()
{
    auto id = currentDeviceId();
    if (id < 0)
        return;

    // Start measurement on the selected device using DigitizerInteractor
    auto r = m_interactor->startMeasure(id);
    logMessage(QString("Id %1: Measurement %2 started").arg(id).arg(r ? "" : "not"));
}

void DeviceControlPanel::onStopMeasure()
{
    auto id = currentDeviceId();
    if (id < 0)
        return;

    // Stop measurement on the selected device using DigitizerInteractor
    auto r = m_interactor->stopMeasure(id);
    logMessage(QString("Id %1: Measurement %2 stopped").arg(id).arg(r ? "" : "not"));
}

void DeviceControlPanel::onClearLog()
{
    m_logEdit->clear();
}

void DeviceControlPanel::onDeviceSelectionChanged()
{
    emit deviceSelectionChanged(currentDeviceId());
}

void DeviceControlPanel::onLogFirmwareSettings()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        logMessage("No device selected");
        return;
    }

    if (!m_interactor->isDeviceConnected(id))
    {
        logMessage(QString("Id %1: Device not connected").arg(id));
        return;
    }

    auto schema = m_interactor->firmwareSettings(id).first;
    
    logMessage(QString("Id %1: Firmware Settings Schema:").arg(id));
    
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(schema.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
    {
        logMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
    }
    else
    {
        logMessage(schema);
    }
}

void DeviceControlPanel::onUploadSettings()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        logMessage("No device selected");
        return;
    }

    if (!m_interactor->isDeviceConnected(id))
    {
        logMessage(QString("Id %1: Device not connected").arg(id));
        return;
    }

    auto result = m_interactor->uploadSettings(id);
    logMessage(QString("Id %1: Settings upload %2").arg(id).arg(result ? "successful" : "failed"));
}

void DeviceControlPanel::onDownloadSettings()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        logMessage("No device selected");
        return;
    }

    if (!m_interactor->isDeviceConnected(id))
    {
        logMessage(QString("Id %1: Device not connected").arg(id));
        return;
    }

    auto result = m_interactor->downloadSettings(id);
    logMessage(QString("Id %1: Settings download %2").arg(id).arg(result ? "successful" : "failed"));
}

