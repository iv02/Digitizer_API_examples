#pragma once

#include "iabstractsettingsmodel.h"

#include <QObject>

namespace client
{
class IAbstractSettingsModel;
enum class ItemState;
} 

namespace device
{

class DeviceManagerBase;

class FirmwareSettingsModelRepositoryBase : public QObject
{
    Q_OBJECT

  signals:
    void firmwareSettingsValidationFailed() const;
    void deviceSettingsChanged(int64_t id, const QVariantList &settings) const;
    void firmwareSettingsShemaReceived(int64_t id) const;
    void firmwareSettingsValueReceived(int64_t id) const;

  public:
    FirmwareSettingsModelRepositoryBase(QObject *parent = nullptr);
    ~FirmwareSettingsModelRepositoryBase() override = default;

    void setDeviceManager(DeviceManagerBase *deviceManager);
    void setFirmwareSchema(int64_t id, const QString &schema);
    void setFirmwareValues(int64_t id, const QString &values) const;

    [[nodiscard]] QStringList getModelList(int64_t id) const;
    [[nodiscard]] QSharedPointer<client::IAbstractSettingsModel> getFirmwareSettingModel(int64_t id, const QString &tabName);
    [[nodiscard]] bool containsDevice(int64_t id) const;
    
    bool validate(int64_t id, const QByteArray &values) const;
    bool validate(const QString &settings, const QString &schema) const;

  protected:
    void updateSettings(int64_t id) const;
    virtual void createFirmwareSettingsTableModel(const int64_t &id, const QString &tabItKey, const QJsonObject &tabItValue, const int &channelsNumber);

  protected:
    struct DeviceModelData
    {
        bool isHasModels;
        std::map<QString, QSharedPointer<client::IAbstractSettingsModel>> models;
    };

    std::map<int64_t, DeviceModelData> m_firmwareSettings;

    DeviceManagerBase *m_deviceManager{nullptr};
};
} // namespace device
