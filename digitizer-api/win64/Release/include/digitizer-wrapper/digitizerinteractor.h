#pragma once

#include <QMap>
#include <QString>
#include <QSharedPointer>
#include <QVector>
#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <functional>

class QAbstractTableModel;

namespace network
{
class EventPacket;
struct EventData;
}

namespace digi
{

enum class FWType
{
    Device = 0,
    PHA,
    PSD,
    WAVEFORM
};

constexpr std::array<const char *, 4> FWTypeName 
{
    "Device",
    "PHA",
    "PSD", 
    "WAVEFORM", 
};

class DigitizerInteractor
{
  public:
    explicit DigitizerInteractor();
    ~DigitizerInteractor();

    /**
     * Attempts to establish connection with a device.
     * If the attempt is successful, fetches the current configuration device and stores it locally for access/modification.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating connection status:
     *         - true: Connection established successfully
     *         - false: Connection failed (check logs for details)
     * @note If device ID doesn't exist, it returns false;
     * @note May block for device handshake timeout period (typically 100-500ms)
     * @see disconnectDevice(), isDeviceConnected()
     */
    bool connectDevice(int64_t id);

    /**
     * Disconnects from the specified device.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating disconnection status:
     *         - true: Disconnection completed successfully
     *         - false: Disconnection failed (device may remain connected)
     * @note if device ID doesn't exist, it returns true;
     * @note if device ID doesn't connected, it returns true;
     * @see connectDevice(), isDeviceConnected()
     */
    bool disconnectDevice(int64_t id);

    /**
     * Checks connection status of a specific device.
     *
     * @param id 64-bit device identifier
     * @return bool Connection status:
     *         - true: Device is connected and responsive
     *         - false: Device is disconnected or unresponsive
     * @note if device is unavailable, it returns false
     * @see connectDevice(), disconnectDevice()
     */
    bool isDeviceConnected(int64_t id) const;

    /**
     * Attempts to start measurement on a device. 
     * If the attempt is successful, device will start measuring and sending data.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating measurement status:
     *         - true: Measurement started successfully
     *         - false: Measurement start failed (check logs for details)
     * @note If device ID doesn't exist, it returns false;
     * @note May block for device handshake timeout period (typically 100-500ms)
     * @see stopMeasure(), isDeviceMeasuring()
     */
    bool startMeasure(int64_t id);

    /**
     * Stops measurement on the specified device.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating measurement stop status:
     *         - true: Measurement stopped successfully
     *         - false: Measurement stop failed (device may remain measuring)
     * @note if device ID doesn't exist, it returns true;
     * @note if device ID doesn't measuring, it returns true;
     * @see startMeasure(), isDeviceMeasuring()
     */
    bool stopMeasure(int64_t id);

    /**
     * Checks measurement status of a specific device.
     *
     * @param id 64-bit device identifier
     * @return bool Measurement status:
     *         - true: Device is measuring
     *         - false: Device is not measuring or unavailable
     * @note if device is unavailable, it returns false
     * @see startMeasure(), stopMeasure()
     */
    bool isDeviceMeasuring(int64_t id) const;

    /**
     * Provides column headers for device information table.
     *
     * @return list of human-readable column titles in display order.
     */
    QList<QString> deviceHeaderLabels() const;

     /**
     * Returns a complete snapshot of currently registered devices.
     *
     * The returned map contains:
     * - Keys: Unique 64-bit integer identifier
     * - Values: Corresponding device parameters as string lists
     *
     * @note String list order corresponds to deviceHeaderLabels() columns.
     * @note Device discovery takes up to 20000 ms to complete
     */
    QMap<int64_t, QList<QString>> devices() const;

    /**
     * Converts firmware type enum to its human-readable name.
     *
     * @param fwType The firmware type enumeration value
     * @return Display name for the firmware type
     */
    QString fwType2Name(FWType fwType) const;

    /**
     * Converts firmware human-readable name to its enum value.
     *
     * @param fwTypeName Localized display name for the firmware type
     * @return Optional FWType:
     *         - Contains value if matched against known types
     *         - Empty if input is blank or no match found
     */
    std::optional<FWType> fwName2Type(QString fwTypeName) const;

    /**
     * Retrieves firmware configuration in JSON format for specified device.
     *
     * @param id 64-bit device identifier
     * @return std::pair<QString, QString> where:
     *         - first: JSON string containing firmware metadata (version, parameters, etc.)
     *         - second: JSON string with current parameter values
     * @note If device id not found, the first and second values are empty.
     */
    std::pair<QString, QString> firmwareSettings(const int64_t &id) const;

    /**
     * Retrieves the number of available channels for a device.
     *
     * @param id Unique 64-bit device identifier
     * @return uint16_t Number of available channels (0 if unknown/invalid)
     * @note If device ID doesn't exist, it returns 0
     */
    uint16_t getDeviceChannels(const int64_t id) const;

    /**
     * Downloads and caches all firmware settings from the specified device.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating operation status:
     *         - true: Settings downloaded and cached successfully
     *         - false: Download failed (check lastError())
     * @note If device id not found, it returns false.
     * @note If device is disconnected, it returns false.
     * @note May block during transfer (typically 100-500ms)
     * @see uploadSettings(), deviceSettings()
     */
    bool downloadSettings(int64_t id);

    /**
     * Uploads cached settings to the specified device.
     *
     * @param id Unique 64-bit device identifier
     * @return bool indicating operation status:
     *         - true: Settings uploaded successfully
     *         - false: Upload failed (check lastError())
     * @note If device ID not found, it returns false
     * @note If device is disconnected, it returns false
     * @note Performs verification of received settings on device
     * @see downloadSettings(), verifySettings()
     */
    bool uploadSettings(int64_t id);

    /**
     * Retrieves supported firmware types for a device.
     *
     * @param id 64-bit device identifier
     * @return std::vector<FWType> List of compatible firmware types
     * @note If device ID not found, it returns empty list
     * @note If device is disconnected, it returns empty list
     */
    std::vector<FWType> fwTypeList(const int64_t &id) const;

    /**
     * Retrieves human-readable names of supported firmware types for a device.
     *
     * @param id 64-bit device identifier
     * @return QStringList Localized firmware type names
     *         Ordered to match fwTypeList() results
     * @note If device ID not found, it returns empty list
     * @note If device is disconnected, it returns empty list
     * @see fwTypeList(), fwType2Name()
     */
    QStringList fwTypeNameList(const int64_t &id) const;

    /**
     * Retrieves settings names for a specific firmware type on a device.
     *
     * @param id 64-bit device identifier
     * @param fwType Firmware type to query
     * @return QStringList Human-readable Case-sensitive setting names in display order
     * @note If device ID not found, it returns empty list
     * @note If device is disconnected, it returns empty list
     */
    QStringList fwSettingList(const int64_t &id, const FWType &fwType) const;

    /**
     * Retrieves settings names for a device's firmware type by name.
     *
     * @param id 64-bit device identifier
     * @param fwTypeName Firmware type name
     * @return QStringList Human-readable Case-sensitive setting names in display order
     * @note If device ID not found, it returns empty list
     * @note If device is disconnected, it returns empty list
     */
    QStringList fwSettingList(const int64_t &id, const QString &fwTypeName) const;

    /**
     * Retrieves firmware setting values with type-specific column interpretation.
     *
     * @param id 64-bit device identifier
     * @param fwType Target firmware type that determines column semantics:
     *        - FWType::Device:
     *          * column 1: Current setting value
     *        - FWType::PHA/PSD/WAVEFORM:
     *          * column 0: Default value
     *          * column 1-N: Channel-specific values (where N = channel count)
     * @param name Case-sensitive setting name
     * @param column Data selector whose meaning depends on fwType
     * @return QVariant containing:
     *         - Requested value if exists
     *         - Invalid QVariant() if setting/column not found
     */
    QVariant getSetting(const int64_t &id, const FWType &fwType, const QString &name, const int &column) const;
    QVariant getSetting(const int64_t &id, const QString &fwTypeName, const QString &name, const int &column) const;

    /**
     * Updates firmware setting values with type-specific column interpretation.
     *
     * @param id 64-bit device identifier
     * @param fwType Target firmware type that determines column semantics:
     *        - FWType::Device:
     *          * column 1: Updates current setting value
     *        - FWType::PHA/PSD/WAVEFORM:
     *          * column 0: Updates default value (affects all channels)
     *          * column 1-N: Updates channel-specific values
     * @param name Case-sensitive setting name
     * @param column Data selector whose meaning depends on fwType
     * @param value New value to set (type must match setting requirements)
     * @return bool indicating success:
     *         - true: Value was successfully updated
     *         - false: Update failed (invalid parameters/range/type)
     */
    bool setSetting(const int64_t &id, const FWType &fwType, const QString &name, const int &column, const QVariant &value) const; 
    bool setSetting(const int64_t &id, const QString &fwTypeName, const QString &name, const int &column, const QVariant &value) const;

    /**
     * Retrieves an editable table model for firmware settings of specified type.
     * 
     * @param id 64-bit device identifier
     * @param fwTypeName Name of the firmware type (case-sensitive match) 
     * @return Pointer to QAbstractTableModel with:
     *         - Rows: settings (ordered as in fwSettingList())
     *         - Columns: [Setting Name, Default Value, Current Value]
     *         - Edit role: Column 1 (Current Value) is editable
     * @note If fwTypeName doesn't match known types, it returns nullptr
     * @note The returned pointer is managed internally - DO NOT DELETE IT
     * @warning Manual deletion will cause application crashes 
     * @see fwTypeNameList(), getSetting(), fwSettingList()
     */
    QAbstractTableModel *fwSettingTableModel(const int64_t &id, const QString &fwTypeName);

    /**
     * Sets callback function for receiving single event data.
     * 
     * @param callback Function to be called when event data is received:
     *        - eventData: Event data containing info and waveform packets
     * @note The callback is called from the device manager thread
     * @note Setting nullptr disables the callback
     * @see setDataBatchCallback()
     */
    void setDataEventCallback(std::function<void(const network::EventData &)> callback);

    /**
     * Sets callback function for receiving batch event data.
     * 
     * @param callback Function to be called when batch event data is received:
     *        - batch: Vector of event data (info and waveform pairs)
     * @note The callback is called from the device manager thread
     * @note Setting nullptr disables the callback
     * @see setDataEventCallback()
     */
    void setDataBatchCallback(std::function<void(const QVector<network::EventData> &batch)> callback);

  private:
    DigitizerInteractor(const DigitizerInteractor &) = delete;
    DigitizerInteractor(DigitizerInteractor &&) = delete;
    DigitizerInteractor &operator=(const DigitizerInteractor &) = delete;
    DigitizerInteractor &operator=(DigitizerInteractor &&) = delete;

  private:
    struct Data;
    std::unique_ptr<Data> m_d;
};

} // namespace digi