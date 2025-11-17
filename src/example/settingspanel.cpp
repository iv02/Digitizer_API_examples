#include "settingspanel.h"
#include "digitizerinteractor.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QSpacerItem>
#include <QTableView>
#include <QVBoxLayout>

using namespace digi;

SettingsPanel::SettingsPanel(DigitizerInteractor *interactor, QWidget *parent)
    : QWidget(parent), m_interactor(interactor)
{
    setupUi();
    setupConnections();
    setupTableStyle();
}

void SettingsPanel::showSettings(int64_t deviceId, const QString &fwTypeName)
{
    m_currentDeviceId = deviceId;
    if (deviceId >= 0)
    {
        showSettingsTable(fwTypeName);
        if (m_currentButton)
        {
            uncheckAllButtons();
            m_currentButton->setChecked(true);
        }
    }
    else
    {
        hideSettings();
    }
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

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_pbDeviceSettings = new QPushButton("Device\n settings"));
    buttonLayout->addWidget(m_pbPHASettings = new QPushButton("PHA\n settings"));
    buttonLayout->addWidget(m_pbPSDSettings = new QPushButton("PSD\n settings"));
    buttonLayout->addWidget(m_pbWAVESettings = new QPushButton("WAVE\n settings"));
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pbDeviceSettings->setCheckable(true);
    m_pbPHASettings->setCheckable(true);
    m_pbPSDSettings->setCheckable(true);
    m_pbWAVESettings->setCheckable(true);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_settingsTable);
    mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(mainLayout);

    m_settingsTable->hide();
}

void SettingsPanel::setupConnections()
{
    connect(m_pbDeviceSettings, &QPushButton::clicked, this, &SettingsPanel::onDeviceSettingsClicked);
    connect(m_pbPHASettings, &QPushButton::clicked, this, &SettingsPanel::onPHASettingsClicked);
    connect(m_pbPSDSettings, &QPushButton::clicked, this, &SettingsPanel::onPSDSettingsClicked);
    connect(m_pbWAVESettings, &QPushButton::clicked, this, &SettingsPanel::onWAVESettingsClicked);
}

void SettingsPanel::setupTableStyle()
{
    m_settingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_settingsTable->horizontalHeader()->setStretchLastSection(true);
    m_settingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_settingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_settingsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_settingsTable->verticalHeader()->setVisible(false);
}

void SettingsPanel::onDeviceSettingsClicked()
{
    if (m_currentDeviceId < 0)
    {
        m_pbDeviceSettings->setChecked(false);
        return;
    }

    uncheckAllButtons();
    m_pbDeviceSettings->setChecked(true);
    m_currentButton = m_pbDeviceSettings;
    showSettingsTable("Device");
}

void SettingsPanel::onPHASettingsClicked()
{
    if (m_currentDeviceId < 0)
    {
        m_pbPHASettings->setChecked(false);
        return;
    }

    uncheckAllButtons();
    m_pbPHASettings->setChecked(true);
    m_currentButton = m_pbPHASettings;
    showSettingsTable("PHA");
}

void SettingsPanel::onPSDSettingsClicked()
{
    if (m_currentDeviceId < 0)
    {
        m_pbPSDSettings->setChecked(false);
        return;
    }

    uncheckAllButtons();
    m_pbPSDSettings->setChecked(true);
    m_currentButton = m_pbPSDSettings;
    showSettingsTable("PSD");
}

void SettingsPanel::onWAVESettingsClicked()
{
    if (m_currentDeviceId < 0)
    {
        m_pbWAVESettings->setChecked(false);
        return;
    }

    uncheckAllButtons();
    m_pbWAVESettings->setChecked(true);
    m_currentButton = m_pbWAVESettings;
    showSettingsTable("WAVEFORM");
}

void SettingsPanel::showSettingsTable(const QString &fwTypeName)
{
    if (m_currentDeviceId < 0)
    {
        m_settingsTable->hide();
        return;
    }

    if (auto model = m_interactor->fwSettingTableModel(m_currentDeviceId, fwTypeName))
    {
        if (m_settingsTable->model() != model)
        {
            m_settingsTable->setModel(model);
        }
        m_settingsTable->show();
    }
    else
    {
        m_settingsTable->hide();
    }
}

void SettingsPanel::uncheckAllButtons()
{
    m_pbDeviceSettings->setChecked(false);
    m_pbPHASettings->setChecked(false);
    m_pbPSDSettings->setChecked(false);
    m_pbWAVESettings->setChecked(false);
    m_currentButton = nullptr;
}

QString SettingsPanel::currentSettingsType() const
{
    if (m_pbPHASettings->isChecked())
        return "PHA";
    else if (m_pbPSDSettings->isChecked())
        return "PSD";
    else if (m_pbWAVESettings->isChecked())
        return "WAVEFORM";
    return "Device";
}

bool SettingsPanel::hasActiveSettings() const
{
    return m_currentButton != nullptr;
}

