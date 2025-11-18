#include "settingspanel.h"
#include "digitizerinteractor.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace digi;

SettingsPanel::SettingsPanel(DigitizerInteractor *interactor, QWidget *parent) : QWidget(parent), m_interactor(interactor)
{
    setupUi();
    setupConnections();
}

void SettingsPanel::showSettings(int64_t deviceId, const QString &fwTypeName)
{
    m_currentDeviceId = deviceId;
    if (deviceId < 0)
    {
        hideSettings();
        return;
    }

    updateChannelComboBox();

    if (!fwTypeName.isEmpty())
    {
        int index = m_fwTypeComboBox->findText(fwTypeName);
        if (index >= 0)
        {
            m_fwTypeComboBox->setCurrentIndex(index);
            m_currentFwTypeName = fwTypeName;
        }
    }
}

void SettingsPanel::hideSettings()
{
    m_currentDeviceId = -1;
    m_currentFwTypeName.clear();
    m_fwTypeComboBox->clear();
    m_channelComboBox->clear();
    clearInputs();
}

void SettingsPanel::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    
    auto historyLabel = new QLabel("История команд:", this);
    m_historyTextEdit = new QTextEdit(this);
    m_historyTextEdit->setReadOnly(true);
    m_historyTextEdit->setMinimumHeight(200);
    
    auto fwTypeLayout = new QHBoxLayout();
    auto fwTypeLabel = new QLabel("Тип прошивки:", this);
    m_fwTypeComboBox = new QComboBox(this);
    m_fwTypeComboBox->setMinimumWidth(300);
    m_refreshFwTypeButton = new QPushButton("Обновить", this);
    fwTypeLayout->addWidget(fwTypeLabel);
    fwTypeLayout->addWidget(m_fwTypeComboBox);
    fwTypeLayout->addWidget(m_refreshFwTypeButton);
    fwTypeLayout->addStretch();
    
    auto inputLayout = new QHBoxLayout();
    auto nameLabel = new QLabel("Имя настройки:", this);
    m_settingNameEdit = new QLineEdit(this);
    auto valueLabel = new QLabel("Значение:", this);
    m_settingValueEdit = new QLineEdit(this);
    auto channelLabel = new QLabel("Канал:", this);
    m_channelComboBox = new QComboBox(this);
    m_channelComboBox->setMinimumWidth(120);
    inputLayout->addWidget(nameLabel);
    inputLayout->addWidget(m_settingNameEdit);
    inputLayout->addWidget(valueLabel);
    inputLayout->addWidget(m_settingValueEdit);
    inputLayout->addWidget(channelLabel);
    inputLayout->addWidget(m_channelComboBox);
    
    auto buttonsLayout = new QHBoxLayout();
    m_setButton = new QPushButton("Set", this);
    m_getButton = new QPushButton("Get", this);
    m_getSettingsListButton = new QPushButton("Get Settings List", this);
    buttonsLayout->addWidget(m_setButton);
    buttonsLayout->addWidget(m_getButton);
    buttonsLayout->addWidget(m_getSettingsListButton);
    buttonsLayout->addStretch();
    
    m_mainLayout->addWidget(historyLabel);
    m_mainLayout->addWidget(m_historyTextEdit);
    m_mainLayout->addLayout(fwTypeLayout);
    m_mainLayout->addLayout(inputLayout);
    m_mainLayout->addLayout(buttonsLayout);
    m_mainLayout->addStretch();
    
    setLayout(m_mainLayout);
}

void SettingsPanel::setupConnections()
{
    connect(m_setButton, &QPushButton::clicked, this, &SettingsPanel::onSetButtonClicked);
    connect(m_getButton, &QPushButton::clicked, this, &SettingsPanel::onGetButtonClicked);
    connect(m_getSettingsListButton, &QPushButton::clicked, this, &SettingsPanel::onGetSettingsListButtonClicked);
    connect(m_fwTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPanel::onFwTypeComboBoxChanged);
    connect(m_refreshFwTypeButton, &QPushButton::clicked, this, &SettingsPanel::onRefreshFwTypeButtonClicked);
}

void SettingsPanel::onSetButtonClicked()
{
    if (m_currentDeviceId < 0)
    {
        appendToHistory("Ошибка: устройство не выбрано");
        return;
    }

    if (m_currentFwTypeName.isEmpty())
    {
        appendToHistory("Ошибка: тип прошивки не выбран");
        return;
    }

    QString settingName = m_settingNameEdit->text().trimmed();
    if (settingName.isEmpty())
    {
        appendToHistory("Ошибка: имя настройки не указано");
        return;
    }

    QString valueStr = m_settingValueEdit->text().trimmed();
    if (valueStr.isEmpty())
    {
        appendToHistory("Ошибка: значение не указано");
        return;
    }

    bool isDevice = (m_currentFwTypeName == "Device");
    int apiColumn;
    if (isDevice)
    {
        apiColumn = 1;
    }
    else
    {
        int channelIndex = m_channelComboBox->currentIndex();
        if (channelIndex < 0)
        {
            appendToHistory("Ошибка: канал не выбран");
            return;
        }
        apiColumn = m_channelComboBox->itemData(channelIndex).toInt();
    }

    QVariant value(valueStr);
    
    bool success = m_interactor->setSetting(m_currentDeviceId, m_currentFwTypeName, settingName, apiColumn, value);
    
    QString channelInfo = isDevice ? "Device" : QString("Канал %1").arg(apiColumn);
    if (success)
    {
        appendToHistory(QString("Set: %1 = %2 (тип: %3, %4) - Успешно")
                       .arg(settingName, valueStr, m_currentFwTypeName, channelInfo));
    }
    else
    {
        appendToHistory(QString("Set: %1 = %2 (тип: %3, %4) - Ошибка")
                       .arg(settingName, valueStr, m_currentFwTypeName, channelInfo));
    }
}

void SettingsPanel::onGetButtonClicked()
{
    if (m_currentDeviceId < 0)
    {
        appendToHistory("Ошибка: устройство не выбрано");
        return;
    }

    if (m_currentFwTypeName.isEmpty())
    {
        appendToHistory("Ошибка: тип прошивки не выбран");
        return;
    }

    QString settingName = m_settingNameEdit->text().trimmed();
    if (settingName.isEmpty())
    {
        appendToHistory("Ошибка: имя настройки не указано");
        return;
    }

    bool isDevice = (m_currentFwTypeName == "Device");
    int apiColumn;
    if (isDevice)
    {
        apiColumn = 1;
    }
    else
    {
        int channelIndex = m_channelComboBox->currentIndex();
        if (channelIndex < 0)
        {
            appendToHistory("Ошибка: канал не выбран");
            return;
        }
        apiColumn = m_channelComboBox->itemData(channelIndex).toInt();
    }

    QVariant value = m_interactor->getSetting(m_currentDeviceId, m_currentFwTypeName, settingName, apiColumn);
    
    QString valueStr = value.toString();
    m_settingValueEdit->setText(valueStr);
    
    QString channelInfo = isDevice ? "Device" : QString("Канал %1").arg(apiColumn);
    appendToHistory(QString("Get: %1 = %2 (тип: %3, %4)")
                   .arg(settingName, valueStr, m_currentFwTypeName, channelInfo));
}

void SettingsPanel::onGetSettingsListButtonClicked()
{
    if (m_currentDeviceId < 0)
    {
        appendToHistory("Ошибка: устройство не выбрано");
        return;
    }

    if (m_currentFwTypeName.isEmpty())
    {
        appendToHistory("Ошибка: тип прошивки не выбран");
        return;
    }

    QStringList settingNames = m_interactor->fwSettingList(m_currentDeviceId, m_currentFwTypeName);
    
    if (settingNames.isEmpty())
    {
        appendToHistory(QString("Get Settings List (тип: %1): список пуст").arg(m_currentFwTypeName));
    }
    else
    {
        appendToHistory(QString("Get Settings List (тип: %1):").arg(m_currentFwTypeName));
        for (const QString &name : settingNames)
        {
            appendToHistory(QString("  - %1").arg(name));
        }
    }
}

void SettingsPanel::onFwTypeComboBoxChanged(int index)
{
    if (index < 0 || m_currentDeviceId < 0)
        return;

    QString fwTypeName = m_fwTypeComboBox->itemText(index);
    m_currentFwTypeName = fwTypeName;
    updateChannelComboBox();
    appendToHistory(QString("Выбран тип прошивки: %1").arg(fwTypeName));
}

void SettingsPanel::updateFwTypeComboBox()
{
    m_fwTypeComboBox->clear();
    
    if (m_currentDeviceId < 0)
    {
        appendToHistory("Ошибка: устройство не выбрано для обновления списка типов прошивки");
        return;
    }

    bool isConnected = m_interactor->isDeviceConnected(m_currentDeviceId);
    appendToHistory(QString("Обновление списка типов прошивки для устройства %1 (подключено: %2)")
                   .arg(m_currentDeviceId).arg(isConnected ? "да" : "нет"));

    QStringList fwTypeNames = m_interactor->fwTypeNameList(m_currentDeviceId);

    if (fwTypeNames.isEmpty())
    {
        appendToHistory("Список типов прошивки пуст");
        if (!isConnected)
        {
            appendToHistory("Попробуйте подключить устройство и обновить список");
        }
        return;
    }

    m_fwTypeComboBox->addItems(fwTypeNames);
    appendToHistory(QString("Добавлено типов прошивки: %1").arg(fwTypeNames.size()));
    
    if (m_fwTypeComboBox->count() > 0 && m_currentFwTypeName.isEmpty())
    {
        m_fwTypeComboBox->setCurrentIndex(0);
        m_currentFwTypeName = m_fwTypeComboBox->currentText();
        appendToHistory(QString("Автоматически выбран тип прошивки: %1").arg(m_currentFwTypeName));
    }
}

void SettingsPanel::updateChannelComboBox()
{
    m_channelComboBox->clear();
    
    if (m_currentDeviceId < 0)
        return;

    bool isDevice = (m_currentFwTypeName == "Device");
    
    if (isDevice)
    {
        m_channelComboBox->setEnabled(false);
        return;
    }
    
    m_channelComboBox->setEnabled(true);
    
    m_channelComboBox->addItem("Default", 0);
    
    uint16_t channels = m_interactor->getDeviceChannels(m_currentDeviceId);
    for (uint16_t ch = 1; ch <= channels; ++ch)
    {
        m_channelComboBox->addItem(QString("Channel %1").arg(ch), static_cast<int>(ch));
    }
    
    if (m_channelComboBox->count() > 0)
    {
        m_channelComboBox->setCurrentIndex(0);
    }
}

void SettingsPanel::appendToHistory(const QString &message)
{
    m_historyTextEdit->append(message);
    QTextCursor cursor = m_historyTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_historyTextEdit->setTextCursor(cursor);
}

void SettingsPanel::clearInputs()
{
    m_settingNameEdit->clear();
    m_settingValueEdit->clear();
}

void SettingsPanel::refreshFwTypeButtons()
{
    if (m_currentDeviceId < 0)
        return;

    updateFwTypeComboBox();

    if (!m_currentFwTypeName.isEmpty())
    {
        int index = m_fwTypeComboBox->findText(m_currentFwTypeName);
        if (index >= 0)
        {
            m_fwTypeComboBox->setCurrentIndex(index);
        }
    }
    
    updateChannelComboBox();
}

void SettingsPanel::onRefreshFwTypeButtonClicked()
{
    if (m_currentDeviceId < 0)
    {
        appendToHistory("Ошибка: устройство не выбрано");
        return;
    }

    QString savedFwTypeName = m_currentFwTypeName;
    updateFwTypeComboBox();

    if (!savedFwTypeName.isEmpty())
    {
        int index = m_fwTypeComboBox->findText(savedFwTypeName);
        if (index >= 0)
        {
            m_fwTypeComboBox->setCurrentIndex(index);
            m_currentFwTypeName = savedFwTypeName;
        }
        else if (m_fwTypeComboBox->count() > 0)
        {
            m_fwTypeComboBox->setCurrentIndex(0);
            m_currentFwTypeName = m_fwTypeComboBox->currentText();
            appendToHistory(QString("Предыдущий тип прошивки '%1' не найден, выбран первый доступный: %2")
                          .arg(savedFwTypeName, m_currentFwTypeName));
        }
    }
    else if (m_fwTypeComboBox->count() > 0)
    {
        m_fwTypeComboBox->setCurrentIndex(0);
        m_currentFwTypeName = m_fwTypeComboBox->currentText();
    }
    
    updateChannelComboBox();
}

QString SettingsPanel::currentSettingsType() const
{
    return m_currentFwTypeName;
}

bool SettingsPanel::hasActiveSettings() const
{
    return m_currentDeviceId >= 0 && !m_currentFwTypeName.isEmpty();
}
