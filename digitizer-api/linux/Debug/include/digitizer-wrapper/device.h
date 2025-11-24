#pragma once

#include "devicestatemachine.h"

#include <QObject>
#include <memory>

namespace device
{

struct Hardware
{
    struct
    {
        uint32_t channels;
        uint32_t sampleRate;
    } adc;

    uint32_t bitResolution;

    struct
    {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
    } protocol;
};

class Device : public QObject
{
    Q_OBJECT

  signals:
    void deviceStateChanged(int64_t id, QFlags<DeviceStateFlag> flags, const QVariantList &settings);
    void deviceCommandEmit(int64_t id, network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

  public:
    Device(QObject *parent = nullptr);

    Device(const Device &other);
    Device &operator=(const Device &other);
    Device(Device &&other) noexcept;
    Device &operator=(Device &&other) noexcept;

    ~Device() override = default;

    void initializeStateMachine();

    //[CONNECTION]
    [[nodiscard]] QString getIpv4() const;
    void setIpv4(const QString &);

    [[nodiscard]] uint16_t getCommandPort() const noexcept;
    void setCommandPort(uint16_t port);

    [[nodiscard]] uint16_t getConnectionPort() const noexcept;
    void setConnectionPort(uint16_t);

    [[nodiscard]] uint16_t getDataPort() const noexcept;
    void setDataPort(uint16_t port);

    //[INFO]
    [[nodiscard]] int64_t getId() const noexcept;
    void setId(int64_t id);

    [[nodiscard]] uint16_t serialNumber() const;
    void setSerialNumber(uint16_t);

    [[nodiscard]] QString fpgaFirmwareVersion() const;
    void setFpgaFirmwareVersion(const QString &);

    [[nodiscard]] QString armFirmwareVersion() const;
    void setArmFirmwareVersion(const QString &);

    [[nodiscard]] QString getName() const;
    void setName(QString name);

    [[nodiscard]] QString getDescription() const;
    void setDescription(const QString &description);

    //[FIRMWARE SETTINGS]
    [[nodiscard]] QString getFirmwareSettingsSchema() const;
    void setFirmwareSettingsSchema(const QString &);

    [[nodiscard]] QString getFirmwareSettingsValues() const;
    void setFirmwareSettingsValues(const QString &);

    [[nodiscard]] bool isFirmwareSettingsReady() const;

    //[HARDWARE]
    [[nodiscard]] Hardware getHardware() const;
    void setHardware(const Hardware &info);

    //[SETTINGS]
    [[nodiscard]] QVariantList getSettings() const;
    void setSettings(const QVariantList &);

  public slots:
    void onDeviceNetworkEvent(network::NETWORK_DEVICE_EVENT event, QVariantList parameters);
    void onDeviceCommandReceived(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters) const;

    [[nodiscard]] QFlags<DeviceStateFlag> stateFlags() const;

  private:
    void handleStateChanged(QFlags<DeviceStateFlag> flags);
    void handleDeviceCommandEmit(network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

    struct
    {
        QString ipv4;
        uint16_t commandPort;
        uint16_t connectionPort;
        uint16_t dataPort;
    } m_connection{};

    struct
    {
        int64_t id;
        uint16_t serialNumber;
        QString fpgaFirmwareVersion;
        QString armFirmwareVersion;
        QString name;
        QString description;
    } m_info{};

    struct
    {
        QString schema;
        QString values;
    } m_firmwareSettings;

    QVariantList m_settings{};
    Hardware m_hardware{};
    std::unique_ptr<DeviceStateMachine> m_stateMachine{};
};

} // namespace device
