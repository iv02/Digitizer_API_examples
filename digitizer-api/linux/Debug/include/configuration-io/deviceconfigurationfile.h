#pragma once

#include <QString>

namespace configuration_io
{

struct ConfigurationData
{
    QString deviceName;
    QString schema;
    QString settings;
};

enum class ReadStatus
{
    Ok = 0,
    OpenError,
    InvalidFormat
};

struct ReadResult
{
    ReadStatus status{ReadStatus::InvalidFormat};
    ConfigurationData data{};
};

class DeviceConfigurationFile final
{
  public:
    static bool write(const QString &path, const ConfigurationData &configuration);
    static ReadResult read(const QString &path);
    static bool validateFile(const QString &path);
};

} // namespace configuration_io
