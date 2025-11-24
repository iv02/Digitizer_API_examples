#pragma once

#include "device.h"

#include <QObject>
#include <QFuture>
#include <QPromise>

#include <api.pb.h>

namespace network 
{
class NetworkMediator;
}

class FirmwareUpdateRequest;

class QThread;

namespace device
{

class FirmwareSettingsModelRepositoryBase;

class DeviceManagerBase: public QObject
{
    Q_OBJECT

  signals:
    // [TO NETWORK]
    void sendCommand(int64_t id, network::NETWORK_DEVICE_COMMAND command, QVariantList parameters);

    //[TO FILE SAVER]
    void settingsChanged(const uint64_t &id, const QFlags<DeviceStateFlag> &flags, const QVariantList &settings);

    //[TO INTERFACE]
    void firmwareSettingsValidationFailed() const;
    void selectedDeviceChanged(const int64_t &id, const QFlags<DeviceStateFlag> &flags, const QVariantList &settingss);
    void deviceStateChanged(const int64_t &id, const QFlags<DeviceStateFlag> &flags, const QVariantList &settings);

    void deviceDetailsOpenRequest(int64_t id);

    void firmwareUpdated(int64_t);
    void firmwareUploadFailed();

    void deviceDiscovered(int64_t);
    void deviceRebootOrLost(int64_t);

    void firmwareSettingsSaved();
    void resetFirmwareSettingsMessage();
    void resetFirmwareSettingsMessageConfirm();

    void updateBitResolution(int64_t id, uint16_t bitResolution);
    void measureStatusChanged(int64_t id, bool started);
    void deviceNetworkEventPopup(int64_t id, network::NETWORK_DEVICE_EVENT event, QVariantList parameters);

    void disconnectDeviceFinished();

  public:
    explicit DeviceManagerBase(QObject *parent = nullptr);
    ~DeviceManagerBase() override;

    virtual void initialize();
    virtual void initializeFirmwareSettingsModelRepository();

  public slots:
    // [DEVICE MANAGEMENT]
    void onDisconnectDevices();
    virtual void connectDevice(int64_t);
    virtual void disconnectDevice(int64_t);
    void updateDevice(int64_t, const FirmwareUpdateRequest &);
    void updateDevice(int64_t, const DeviceRootFSUpdateRequest &);
    void resetDevice(int64_t);

    void setDeviceMeasurementValues(int64_t, const QString &);

    // [CONNECTIONS]
    [[nodiscard]]QFlags<DeviceStateFlag> stateFlags(int64_t) const;
    [[nodiscard]] std::optional<uint16_t> getDeviceDataPort(int64_t) const;

    // [INFO]
    [[nodiscard]] QString getDeviceSerialNumber(int64_t) const;
    [[nodiscard]] QString getDeviceFPGAFirmware(int64_t) const;
    [[nodiscard]] QString getDeviceArmFirmware(int64_t) const;
    [[nodiscard]] QString getDeviceName(int64_t) const;

    // [HARDWARE]
    [[nodiscard]] uint16_t getDeviceChannels(int64_t) const;
    [[nodiscard]] uint16_t getDeviceSampleRate(int64_t) const;
    [[nodiscard]] uint16_t getDeviceBitResolution(int64_t) const;
    [[nodiscard]] QString getDeviceProtocolVersion(int64_t) const;

    // [MEASUREMENT SETTINGS]
    [[nodiscard]] QVariantList getDeviceSettings(const int64_t &) const;
    void setSettings(const int64_t &id, const QVariantList &settings);

    // [FIRMWARE SETTINGS]
    [[nodiscard]] bool isFirmwareSettingsReady(int64_t) const;
    [[nodiscard]] std::pair<QString, QString> firmwareSettings(const int64_t &id) const;

    // [DEVICE MANAGEMENT]
    [[nodiscard]] std::vector<int64_t> knownDeviceIds() const;
    std::vector<int64_t> connectedDeviceIds();
    [[nodiscard]] bool deviceIsKnown(int64_t) const;
    [[nodiscard]] int64_t selectedDeviceId() const;

    // [INSTANCES]
    [[nodiscard]] FirmwareSettingsModelRepositoryBase *getFirmwareSettingsModelRepository() const;

    //void startEvents(int64_t);
    void stopEvents(int64_t);
    bool isHasConnectedDevice();
    bool isHasMeasuringDevice();

    void onDeviceSelected(int64_t id) noexcept;

  protected slots:
    virtual void onDeviceNetworkEvent(int64_t id, network::NETWORK_DEVICE_EVENT event, QVariantList parameters);
    void onDeviceRebootOrLost(int64_t id);

  protected:
    [[nodiscard]] Device *findDevice(int64_t id) noexcept;
    [[nodiscard]] const Device *findDevice(int64_t id) const noexcept;
    void executeDeviceCommand(int64_t id, network::NETWORK_DEVICE_COMMAND command, const QVariantList &params = {});

    network::NetworkMediator *m_networkMediator{nullptr};
    QThread *m_networkThread{nullptr};

    FirmwareSettingsModelRepositoryBase *m_firmwareSettingsModelRepository{nullptr};

    int64_t m_selectedDeviceId{-1};
    std::map<int64_t, Device> m_devices;

  public:
    // [THREAD-SAFE ACCESS]
    int64_t selectedDeviceIdSafe() const
    {
        return invokeInManagerThread<int64_t>([this] { return selectedDeviceId(); });
    }

    template <typename T> T invokeInManagerThread(std::function<T()> method) const
    {
        if (QThread::currentThread() == this->thread())
            return method();

        QPromise<T> promise;

        QObject *nonConstSelf = const_cast<QObject *>(static_cast<const QObject *>(this));

        QMetaObject::invokeMethod(
            nonConstSelf,
            [&promise, method] {
                promise.start();
                promise.addResult(method());
                promise.finish();
            },
            Qt::QueuedConnection);

        promise.future().waitForFinished();
        return promise.future().result();
    }
};

} // namespace device

Q_DECLARE_METATYPE(int64_t)
