#include "devicecontrolpanel.h"
#include "digitizerinteractor.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

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
    onDiscoverDevices();
}

void DeviceControlPanel::appendLog(const QString &text)
{
    m_logEdit->append(text);
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

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_pbDiscoverDevices = new QPushButton("Discovered\n devices"));
    buttonLayout->addWidget(m_pbConnectDevice = new QPushButton("Connect\n device"));
    buttonLayout->addWidget(m_pbDisConnectDevice = new QPushButton("Disconnect\n device"));
    buttonLayout->addWidget(m_pbStartMeasure = new QPushButton("Start\n measure"));
    buttonLayout->addWidget(m_pbStopMeasure = new QPushButton("Stop\n measure"));
    buttonLayout->addWidget(m_pbClearLog = new QPushButton("Clear\n log"));
    buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
}

void DeviceControlPanel::setupConnections()
{
    connect(m_pbDiscoverDevices, &QPushButton::clicked, this, &DeviceControlPanel::onDiscoverDevices);
    connect(m_pbConnectDevice, &QPushButton::clicked, this, &DeviceControlPanel::onConnectDevice);
    connect(m_pbDisConnectDevice, &QPushButton::clicked, this, &DeviceControlPanel::onDisconnectDevice);
    connect(m_pbStartMeasure, &QPushButton::clicked, this, &DeviceControlPanel::onStartMeasure);
    connect(m_pbStopMeasure, &QPushButton::clicked, this, &DeviceControlPanel::onStopMeasure);
    connect(m_pbClearLog, &QPushButton::clicked, this, &DeviceControlPanel::onClearLog);
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

void DeviceControlPanel::onDiscoverDevices()
{
    auto id = m_devicesModel->data(m_devicesModel->index(m_devicesTable->currentIndex().row(), 1));

    m_devices = m_interactor->devices();

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

    m_logEdit->append(QString("Discover: count device = %1").arg(m_devices.size()));
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    onDeviceSelectionChanged();
}

void DeviceControlPanel::onConnectDevice()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        return;
    }

    auto r = m_interactor->connectDevice(id);
    auto channels = QString("; Channels = ") + QString::number(m_interactor->getDeviceChannels(id));
    m_logEdit->append(QString("Id %1: Device %2 connected").arg(id).arg(r ? "" : "not") + channels);
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    onDiscoverDevices();
}

void DeviceControlPanel::onDisconnectDevice()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        return;
    }

    auto r = m_interactor->disconnectDevice(id);
    m_logEdit->append(QString("Id %1: Device %2 disconnected").arg(id).arg(r ? "" : "not"));
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    onDiscoverDevices();
}

void DeviceControlPanel::onStartMeasure()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        return;
    }

    auto r = m_interactor->startMeasure(id);
    m_logEdit->append(QString("Id %1: Measurement %2 started").arg(id).arg(r ? "" : "not"));
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    onDiscoverDevices();
}

void DeviceControlPanel::onStopMeasure()
{
    auto id = currentDeviceId();
    if (id < 0)
    {
        return;
    }

    auto r = m_interactor->stopMeasure(id);
    m_logEdit->append(QString("Id %1: Measurement %2 stopped").arg(id).arg(r ? "" : "not"));
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    onDiscoverDevices();
}

void DeviceControlPanel::onClearLog()
{
    m_logEdit->clear();
}

void DeviceControlPanel::onDeviceSelectionChanged()
{
    emit deviceSelectionChanged(currentDeviceId());
}

