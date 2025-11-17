
#include "mainwindow.h"
#include "digitizerinteractor.h"

#include <QHeaderView>
#include <QJsonDocument>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBlock>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace digi;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    setStyle();
    setConnect();
}

MainWindow::~MainWindow()
{
    delete m_digitizerInteractor;
    delete m_devicesModel;
}

void MainWindow::setupUi()
{
    setWindowTitle("Test fw-settings-wrapper");
    setMinimumSize(1000, 800);

    m_digitizerInteractor = new digi::DigitizerInteractor();

    auto splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(setupLeftPanelUi());
    splitter->addWidget(setupRightPanelUi());

    auto centralWidget = new QWidget(this);
    centralWidget->setLayout(new QVBoxLayout());
    setCentralWidget(centralWidget);
    centralWidget->layout()->addWidget(splitter);
}

QWidget *MainWindow::setupLeftPanelUi()
{
    m_devicesModel = new QStandardItemModel(this);

    auto headerLabels = m_digitizerInteractor->deviceHeaderLabels();
    m_devicesModel->setColumnCount(static_cast<int>(headerLabels.size()));
    m_devicesModel->setHorizontalHeaderLabels(headerLabels);

    m_devicesTable = new QTableView(this);
    m_devicesTable->setModel(m_devicesModel);

    m_edit = new QTextEdit(this);
    m_edit->setLineWrapMode(QTextEdit::NoWrap);

    auto splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_edit);
    splitter->addWidget(m_devicesTable);
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 2);

    m_menuFWSettingList = new QMenu(this);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pbDiscoverDevices = new QPushButton("Discovered\n devices"));
    layout->addWidget(m_pbConnectDevice = new QPushButton("Connect\n device"));
    layout->addWidget(m_pbDisConnectDevice = new QPushButton("Disconnect\n device"));
    layout->addWidget(m_pbDownloadSettings = new QPushButton("Download\n settings"));
    layout->addWidget(m_pbUploadSettings = new QPushButton("Upload\n settings"));
    layout->addWidget(m_pbDeviceSettingList = new QPushButton("FWType\n list"));
    layout->addWidget(m_pbFwSettings = new QPushButton("FwSettings\n list"));
    layout->addWidget(m_pbSetSettings = new QPushButton("Set\n FwSettings"));

    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    auto layoutMain = new QVBoxLayout();
    layoutMain->addLayout(layout);
    layoutMain->addWidget(splitter);

    auto centralWidget = new QWidget(this);
    centralWidget->setLayout(layoutMain);
    return centralWidget;
}

QWidget *MainWindow::setupRightPanelUi()
{
    m_dhsfTable = new QTableView(this);
    m_editSV = new QTextEdit(this);
    m_editSV->setLineWrapMode(QTextEdit::NoWrap);
    m_editSV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *layout = new QHBoxLayout();

    layout->addWidget(m_pbDeviceSettings = new QPushButton("Device\n settings"));
    layout->addWidget(m_pbPHASettings = new QPushButton("PHA\n settings"));
    layout->addWidget(m_pbPSDSettings = new QPushButton("PSD\n settings"));
    layout->addWidget(m_pbWAVESettings = new QPushButton("WAVE\n settings"));
    layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pbSettings = nullptr;
    m_pbDeviceSettings->setCheckable(true);
    m_pbPHASettings->setCheckable(true);
    m_pbPSDSettings->setCheckable(true);
    m_pbWAVESettings->setCheckable(true);

    connect(m_pbDeviceSettings, &QPushButton::clicked, this, [this](bool checked) { onShowLeftPanel(checked, m_pbDeviceSettings); });
    connect(m_pbPHASettings, &QPushButton::clicked, this, [this](bool checked) { onShowLeftPanel(checked, m_pbPHASettings); });
    connect(m_pbPSDSettings, &QPushButton::clicked, this, [this](bool checked) { onShowLeftPanel(checked, m_pbPSDSettings); });
    connect(m_pbWAVESettings, &QPushButton::clicked, this, [this](bool checked) { onShowLeftPanel(checked, m_pbWAVESettings); });

    auto layoutMain = new QVBoxLayout();
    layoutMain->addLayout(layout);
    layoutMain->addWidget(m_editSV);
    layoutMain->addWidget(m_dhsfTable);
    layoutMain->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    layoutMain->setStretch(1, 0);
    layoutMain->setStretch(1, 100);
    layoutMain->setStretch(2, 100);
    layoutMain->setStretch(2, 1);

    m_editSV->hide();
    m_dhsfTable->hide();

    auto rightPanelWidget = new QWidget(this);
    rightPanelWidget->setLayout(layoutMain);
    return rightPanelWidget;
}

void MainWindow::onDiscoverDevices()
{
    auto id = m_devicesModel->data(m_devicesModel->index(m_devicesTable->currentIndex().row(), 1));

    m_devices = m_digitizerInteractor->devices();

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
    else
    {
        m_devicesTable->selectRow(0);
        m_devicesTable->scrollTo(m_devicesTable->model()->index(0, 0));
    }
    onShowLeftPanel();
}

void MainWindow::setConnect()
{
    connect(m_pbSetSettings, &QPushButton::clicked, this, [this]() {
        m_edit->clear();

        auto id = idCurrentDevice();
        if (id < 0)
            return;

        auto editAppendValue = [this](const int64_t &id, const auto &fwTypeName, const auto &nameValue, const int &column) {
            auto value = m_digitizerInteractor->getSetting(id, fwTypeName, nameValue, column).toString();
            m_edit->append(QString("- %1::%2 = %3").arg(fwTypeName, nameValue, value));
        };

        m_edit->append("Old value:");
        editAppendValue(id, "Device", "fpgaFirmwares", 1);
        editAppendValue(id, "Device", "fanControl", 1);

        Q_UNUSED(m_digitizerInteractor->setSetting(id, "Device", "fpgaFirmwares", 1, 0));
        Q_UNUSED(m_digitizerInteractor->setSetting(id, "Device", "fanControl", 1, "true"));

        m_edit->append("New value:");
        editAppendValue(id, "Device", "fpgaFirmwares", 1);
        editAppendValue(id, "Device", "fanControl", 1);

        m_dhsfTable->update();
    });

    connect(m_pbDiscoverDevices, &QPushButton::clicked, this, [this]() {
        onDiscoverDevices();
        m_edit->clear();
        m_edit->append(QString("Discover: count device = %1").arg(m_devices.size()));
    });

    connect(m_pbConnectDevice, &QPushButton::clicked, this, [this, callback = &DigitizerInteractor::connectDevice]() {
        if (auto id = idCurrentDevice(); id >= 0)
        {
            auto r = (m_digitizerInteractor->*callback)(id);

            auto channels = QString("; Channels = ") + QString::number(m_digitizerInteractor->getDeviceChannels(id));

            m_edit->setPlainText(QString("Id %1: Device %2 connected").arg(id).arg(r ? "" : "not") + channels);

            onDiscoverDevices();
        }
        else
            m_edit->clear();
    });

    connect(m_pbDisConnectDevice, &QPushButton::clicked, this, [this, callback = &DigitizerInteractor::disconnectDevice]() {
        if (auto id = idCurrentDevice(); id >= 0)
        {
            auto r = (m_digitizerInteractor->*callback)(id);
            m_edit->setPlainText(QString("Id %1: Device %2 disconnected").arg(id).arg(r ? "" : "not"));
            onDiscoverDevices();
        }
        else
            m_edit->clear();
    });

    connect(m_pbDownloadSettings, &QPushButton::clicked, this, [this, callback = &DigitizerInteractor::downloadSettings]() {
        if (auto id = idCurrentDevice(); id >= 0)
        {
            auto r = (m_digitizerInteractor->*callback)(id);
            m_edit->setPlainText(QString("Id %1: Device settings %2 downloaded").arg(id).arg(r ? "" : "not"));
            onDiscoverDevices();
        }
        else
            m_edit->clear();
    });

    connect(m_pbUploadSettings, &QPushButton::clicked, this, [this, callback = &DigitizerInteractor::uploadSettings]() {
        if (auto id = idCurrentDevice(); id >= 0)
        {
            auto r = (m_digitizerInteractor->*callback)(id);
            m_edit->setPlainText(QString("Id %1: Device settings %2 uploaded").arg(id).arg(r ? "" : "not"));
            onDiscoverDevices();
        }
        else
            m_edit->clear();
    });

    connect(m_pbDeviceSettingList, &QPushButton::clicked, this, [this, callback = &DigitizerInteractor::fwTypeList]() {
        if (auto id = idCurrentDevice(); id >= 0)
        {
            auto fwTypeList = (m_digitizerInteractor->*callback)(id);

            QStringList strList;
            for (auto fwType : fwTypeList)
                strList.append(m_digitizerInteractor->fwType2Name(fwType));

            m_edit->setPlainText(QString("Id %1: Type settings list:\n %2").arg(id).arg(strList.isEmpty() ? "-" : strList.join(", ")));
            onDiscoverDevices();
        }
        else
            m_edit->clear();
    });

    connect(m_pbFwSettings, &QPushButton::clicked, this, &MainWindow::createFWSettingListMenu);

    connect(m_devicesTable->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex current, const QModelIndex &previous) {
        Q_UNUSED(current)
        onShowLeftPanel();
    });
}

void MainWindow::createFWSettingListMenu()
{
    m_menuFWSettingList->clear();
    for (auto fwTypeName : digi::FWTypeName)
    {
        auto action = m_menuFWSettingList->addAction(fwTypeName);
        action->setCheckable(false);
        connect(action, &QAction::triggered, this, [this, id = idCurrentDevice(), name = fwTypeName]() {
            auto strList = m_digitizerInteractor->fwSettingList(id, name);
            m_edit->clear();
            m_edit->setText(QString("Id %1: FW %2 settings list:\n\n").arg(id).arg(name));
            for (int n = 0; const auto &item : strList)
                appendText(m_edit, QString::number(++n).rightJustified(2) + ". " + item + "\n", 150);
        });
    }
    if (auto button = qobject_cast<QPushButton *>(sender()))
        m_menuFWSettingList->exec(button->mapToGlobal(QPoint(0, button->height())));
}

int64_t MainWindow::idCurrentDevice() const noexcept
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

void MainWindow::onShowLeftPanel(bool checked, QPushButton *pbSender)
{
    auto hideLeftPanel = [this] {
        m_editSV->hide();
        m_dhsfTable->hide();
        m_editSV->clear();
    };

    if (!pbSender && !m_pbSettings)
    {
        hideLeftPanel();
        return;
    }

    if (!pbSender)
    {
        pbSender = m_pbSettings;
        checked = m_pbSettings->isChecked();
    }

    if (m_pbSettings && pbSender != m_pbSettings)
    {
        m_pbSettings->setChecked(false);
        m_pbSettings->repaint();
    }

    if (checked == false)
    {
        m_pbSettings = nullptr;
        hideLeftPanel();
        return;
    }
    else
        m_pbSettings = pbSender;

    int64_t id = idCurrentDevice();
    auto device = m_devices.contains(id) ? &m_devices[id] : nullptr;

    if (!device)
    {
        hideLeftPanel();
        return;
    }

    if (m_pbDeviceSettings == pbSender)
    {
        m_editSV->hide();
        if (auto model = m_digitizerInteractor->fwSettingTableModel(id, "Device"))
        {
            if (m_dhsfTable->model() != model)
            {
                m_dhsfTable->setModel(model);
            }
            m_dhsfTable->show();
        }
        else
            m_dhsfTable->hide();
    }

    else if (m_pbPHASettings == pbSender)
    {
        if (auto model = m_digitizerInteractor->fwSettingTableModel(id, "PHA"))
        {
            if (m_dhsfTable->model() != model)
            {
                m_dhsfTable->setModel(model);
            }
            m_dhsfTable->show();
        }
        else
            m_dhsfTable->hide();
        m_editSV->hide();
    }

    else if (m_pbPSDSettings == pbSender)
    {
        if (auto model = m_digitizerInteractor->fwSettingTableModel(id, "PSD"))
        {
            if (m_dhsfTable->model() != model)
            {
                m_dhsfTable->setModel(model);
            }
            m_dhsfTable->show();
        }
        else
            m_dhsfTable->hide();
        m_editSV->hide();
    }

    else if (m_pbWAVESettings == pbSender)
    {
        if (auto model = m_digitizerInteractor->fwSettingTableModel(id, "WAVEFORM"))
        {
            if (m_dhsfTable->model() != model)
            {
                m_dhsfTable->setModel(model);
            }
            m_dhsfTable->show();
        }
        else
            m_dhsfTable->show();
        m_editSV->hide();
    }
    else
        hideLeftPanel();
}

void MainWindow::appendText(QTextEdit *edit, const QString &text, int percent)
{
    QTextCursor cursor(edit->document());
    cursor.movePosition(QTextCursor::End);

    QTextBlockFormat format;
    format.setLineHeight(percent, QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(format);

    cursor.insertText(text);
}

void MainWindow::setStyle()
{
    m_devicesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_devicesTable->horizontalHeader()->setStretchLastSection(true);
    m_devicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_devicesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_devicesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_devicesTable->verticalHeader()->setVisible(false);
    m_devicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_dhsfTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_dhsfTable->horizontalHeader()->setStretchLastSection(true);
    m_dhsfTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dhsfTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_dhsfTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_dhsfTable->verticalHeader()->setVisible(false);

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
