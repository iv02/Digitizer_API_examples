#include "settingspanel.h"
#include "digitizerinteractor.h"

#include <QHeaderView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

using namespace digi;

SettingsPanel::SettingsPanel(DigitizerInteractor *interactor, QWidget *parent) : QWidget(parent), m_interactor(interactor)
{
    m_settingsModel = new QStandardItemModel(this);
    setupUi();
    setupConnections();
    setupTableStyle();
}

void SettingsPanel::showSettings(int64_t deviceId, const QString &fwTypeName)
{
    m_currentDeviceId = deviceId;
    if (deviceId < 0)
    {
        hideSettings();
        return;
    }

    updateFwTypeButtons();
    uncheckAllButtons();

    for (auto button : m_fwTypeButtons)
    {
        if (button && button->property("fwTypeName").toString() == fwTypeName)
        {
            button->setChecked(true);
            m_currentButton = button;
            break;
        }
    }

    showSettingsTable(fwTypeName);
}

void SettingsPanel::hideSettings()
{
    m_settingsTable->hide();
    m_currentDeviceId = -1;
    uncheckAllButtons();
}

void SettingsPanel::setupUi()
{
    m_settingsTable = new QTableView(this);
    m_settingsTable->setModel(m_settingsModel);

    m_buttonLayout = new QHBoxLayout();

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(m_buttonLayout);
    mainLayout->addWidget(m_settingsTable);
    mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(mainLayout);

    m_settingsTable->hide();
}

void SettingsPanel::setupConnections()
{
    connect(m_settingsModel, &QStandardItemModel::itemChanged, this, [this](QStandardItem *item) {
        if (item && m_currentDeviceId >= 0 && !m_currentFwTypeName.isEmpty())
        {
            int modelColumn = item->column();

            if (modelColumn == 0)
                return;

            int row = item->row();
            QString settingName = m_settingsModel->item(row, 0)->text();

            QVariant value = item->data(Qt::EditRole);
            QVariant oldValue = item->data(Qt::UserRole);

            if (value == oldValue)
                return;

            bool isDevice = (m_currentFwTypeName == "Device");
            int apiColumn;
            if (isDevice)
            {
                apiColumn = 1;
            }
            else
            {
                if (modelColumn == 1)
                    apiColumn = 0;
                else
                    apiColumn = modelColumn - 1;
            }

            // Set setting value using DigitizerInteractor
            bool success = m_interactor->setSetting(m_currentDeviceId, m_currentFwTypeName, settingName, apiColumn, value);
            if (!success)
            {
                m_settingsModel->blockSignals(true);
                item->setData(oldValue, Qt::EditRole);
                m_settingsModel->blockSignals(false);
            }
            else
            {
                item->setData(value, Qt::UserRole);
            }
        }
    });
}

void SettingsPanel::setupTableStyle()
{
    m_settingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_settingsTable->horizontalHeader()->setStretchLastSection(true);
    m_settingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_settingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_settingsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_settingsTable->verticalHeader()->setVisible(false);
    m_settingsTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
}

void SettingsPanel::onFwTypeButtonClicked()
{
    if (m_currentDeviceId < 0)
    {
        if (auto button = qobject_cast<QPushButton *>(sender()))
            button->setChecked(false);
        return;
    }

    auto button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    QString fwTypeName = button->property("fwTypeName").toString();
    if (fwTypeName.isEmpty())
        return;

    uncheckAllButtons();
    button->setChecked(true);
    m_currentButton = button;
    showSettingsTable(fwTypeName);
}

void SettingsPanel::showSettingsTable(const QString &fwTypeName)
{
    if (m_currentDeviceId < 0)
    {
        m_settingsTable->hide();
        return;
    }

    m_currentFwTypeName = fwTypeName;
    updateSettingsModel(fwTypeName);
    m_settingsTable->show();
}

void SettingsPanel::updateSettingsModel(const QString &fwTypeName)
{
    m_settingsModel->clear();

    // Get list of settings for the firmware type using DigitizerInteractor
    QStringList settingNames = m_interactor->fwSettingList(m_currentDeviceId, fwTypeName);
    if (settingNames.isEmpty())
    {
        m_settingsTable->hide();
        return;
    }

    bool isDevice = (fwTypeName == "Device");

    if (isDevice)
    {
        m_settingsModel->setColumnCount(2);
        m_settingsModel->setHorizontalHeaderLabels(QStringList() << "Setting Name"
                                                                 << "Value");
    }
    else
    {
        // Get number of channels to determine column count
        uint16_t channels = m_interactor->getDeviceChannels(m_currentDeviceId);
        m_settingsModel->setColumnCount(static_cast<int>(channels) + 1);
        QStringList headers;
        headers << "Setting Name"
                << "Default";
        for (uint16_t ch = 1; ch <= channels; ++ch)
            headers << QString("Ch %1").arg(ch);
        m_settingsModel->setHorizontalHeaderLabels(headers);
    }

    m_settingsModel->setRowCount(settingNames.size());

    for (int row = 0; row < settingNames.size(); ++row)
    {
        QString settingName = settingNames[row];

        // Setting name column (not editable)
        auto nameItem = new QStandardItem(settingName);
        nameItem->setEditable(false);
        m_settingsModel->setItem(row, 0, nameItem);

        // Value columns
        if (isDevice)
        {
            // Get current value for Device type (column 1)
            QVariant value = m_interactor->getSetting(m_currentDeviceId, fwTypeName, settingName, 1);
            auto valueItem = new QStandardItem(value.toString());
            valueItem->setEditable(true);
            valueItem->setData(value, Qt::UserRole);
            m_settingsModel->setItem(row, 1, valueItem);
        }
        else
        {
            // Get default value (column 0)
            QVariant defaultValue = m_interactor->getSetting(m_currentDeviceId, fwTypeName, settingName, 0);
            auto defaultItem = new QStandardItem(defaultValue.toString());
            defaultItem->setEditable(true);
            defaultItem->setData(defaultValue, Qt::UserRole);
            m_settingsModel->setItem(row, 1, defaultItem);

            // Get channel-specific values
            uint16_t channels = m_interactor->getDeviceChannels(m_currentDeviceId);
            for (uint16_t ch = 0; ch < channels; ++ch)
            {
                QVariant chValue = m_interactor->getSetting(m_currentDeviceId, fwTypeName, settingName, static_cast<int>(ch + 1));
                auto chItem = new QStandardItem(chValue.toString());
                chItem->setEditable(true);
                chItem->setData(chValue, Qt::UserRole);
                m_settingsModel->setItem(row, static_cast<int>(ch + 2), chItem);
            }
        }
    }
}

void SettingsPanel::updateFwTypeButtons()
{
    // Clear existing buttons
    m_currentButton = nullptr;
    for (auto button : m_fwTypeButtons)
    {
        button->deleteLater();
    }
    m_fwTypeButtons.clear();

    if (m_currentDeviceId < 0)
        return;

    // Check if device is connected before getting firmware types
    if (!m_interactor->isDeviceConnected(m_currentDeviceId))
        return;

    // Get available firmware types for the device using DigitizerInteractor
    QStringList fwTypeNames = m_interactor->fwTypeNameList(m_currentDeviceId);

    if (fwTypeNames.isEmpty())
        return;

    while (m_buttonLayout->count() > 0)
    {
        QLayoutItem *item = m_buttonLayout->takeAt(0);
        if (item)
        {
            if (item->widget())
                delete item->widget();
            delete item;
        }
    }

    for (const QString &fwTypeName : fwTypeNames)
    {
        auto button = new QPushButton(fwTypeName + "\n settings", this);
        button->setCheckable(true);
        button->setProperty("fwTypeName", fwTypeName);
        connect(button, &QPushButton::clicked, this, &SettingsPanel::onFwTypeButtonClicked);
        m_buttonLayout->addWidget(button);
        m_fwTypeButtons.append(button);
    }

    m_buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void SettingsPanel::refreshFwTypeButtons()
{
    if (m_currentDeviceId < 0)
        return;

    updateFwTypeButtons();

    if (!m_currentFwTypeName.isEmpty())
    {
        uncheckAllButtons();
        for (auto button : m_fwTypeButtons)
        {
            if (button && button->property("fwTypeName").toString() == m_currentFwTypeName)
            {
                button->setChecked(true);
                m_currentButton = button;
                break;
            }
        }
    }
}

void SettingsPanel::uncheckAllButtons()
{
    for (auto button : m_fwTypeButtons)
    {
        if (button)
            button->setChecked(false);
    }
    m_currentButton = nullptr;
}

QString SettingsPanel::currentSettingsType() const
{
    if (m_currentButton)
        return m_currentButton->property("fwTypeName").toString();
    return m_currentFwTypeName;
}

bool SettingsPanel::hasActiveSettings() const
{
    return m_currentButton != nullptr;
}
