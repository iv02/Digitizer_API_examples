#pragma once

#pragma once

#include "device.h"
#include "devicemanagerbase.h"
#include "packetwrappers/eventdata.h"

#include <QAbstractTableModel>
#include <QObject>
#include <QVariantList>
#include <functional>

#include <api.pb.h>

class QThread;

namespace client
{
enum class DataSource;
}

namespace network
{
class NetworkMediator;
class EventPacket;
} // namespace network

namespace device
{

class FirmwareSettingsModelRepositoryBase;

class DeviceManagerWrapper final : public device::DeviceManagerBase
{
    Q_OBJECT

  public:
    explicit DeviceManagerWrapper(QObject *parent = nullptr);
    ~DeviceManagerWrapper() override;

    void initialize() override;

    void setDataEventCallback(const std::function<void(const network::EventData &)> &callback);
    void setDataBatchCallback(const std::function<void(const QVector<network::EventData> &batch)> &callback);
    void setDeviceDiscoveryCallback(const std::function<void(int64_t deviceId)> &callback);

  public slots:
    QList<QString> deviceHeaderLabels() const;
    QMap<int64_t, QList<QString>> devices() const;

    bool tryConnectDevice(int64_t);
    bool tryDisconnectDevice(int64_t);
    bool isDeviceConnected(int64_t) const;

    bool tryStartMeasure(int64_t);
    bool tryStopMeasure(int64_t);
    bool isDeviceMeasuring(int64_t) const;

    bool downloadSettings(int64_t id);
    bool uploadSettings(int64_t id);

    QStringList tabNameList(int64_t id) const;
    QStringList fwSettingList(int64_t id, QString fwTypeName) const;

    QVariant getSetting(int64_t id, QString fwTypeName, QString name, int column) const;
    bool setSetting(int64_t id, QString fwTypeName, const QString &name, int column, const QVariant &value);

    QAbstractTableModel *fwSettingTableModel(int64_t id, QString tabName) const;

  private slots:
    void onDataReceivedEvent(client::DataSource source, const QSharedPointer<network::EventPacket> &info,
                             const QSharedPointer<network::EventPacket> &waveform) const;
    void onDataReceivedBatch(client::DataSource source, const QVector<network::EventData> &batch) const;

  private slots:
    void onDeviceDiscovered(int64_t deviceId);

  private:
    std::function<void(const network::EventData &)> m_dataEventCallback;
    std::function<void(const QVector<network::EventData> &batch)> m_dataBatchCallback;
    std::function<void(int64_t deviceId)> m_deviceDiscoveryCallback;
};

} // namespace device
