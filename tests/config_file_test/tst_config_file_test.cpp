#include <QtTest>

#include "deviceconfigurationfile.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QVector>

namespace
{

configuration_io::ConfigurationData testConfiguration()
{
    return {QStringLiteral("CI test device"),
            QStringLiteral(R"({"properties":{"Device":{"type":"object","properties":{}}}})"),
            QStringLiteral(R"({"Device":{"Name":"CI test device"}})")};
}

} // namespace

class ConfigFileTest : public QObject
{
    Q_OBJECT

  private slots:
    void readsValidConfigurationFileWithoutDevice();
    void rejectsInvalidConfigurationFileWithoutDevice();
    void reportsMissingConfigurationFileWithoutDevice();
};

void ConfigFileTest::readsValidConfigurationFileWithoutDevice()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString path = temporaryDirectory.filePath(QStringLiteral("valid.dconf"));
    QVERIFY(configuration_io::DeviceConfigurationFile::write(path, testConfiguration()));

    const auto result = configuration_io::DeviceConfigurationFile::read(path);
    QCOMPARE(result.status, configuration_io::ReadStatus::Ok);
    QCOMPARE(result.data.deviceName, QStringLiteral("CI test device"));
    QCOMPARE(result.data.schema, testConfiguration().schema);
    QCOMPARE(result.data.settings, testConfiguration().settings);
}

void ConfigFileTest::rejectsInvalidConfigurationFileWithoutDevice()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString path = temporaryDirectory.filePath(QStringLiteral("invalid.dconf"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(file.write("not a dconf file") > 0);
    file.close();

    const auto result = configuration_io::DeviceConfigurationFile::read(path);
    QCOMPARE(result.status, configuration_io::ReadStatus::InvalidFormat);
}

void ConfigFileTest::reportsMissingConfigurationFileWithoutDevice()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString path = temporaryDirectory.filePath(QStringLiteral("missing.dconf"));
    const auto result = configuration_io::DeviceConfigurationFile::read(path);
    QCOMPARE(result.status, configuration_io::ReadStatus::OpenError);
}

int main(int argc, char *argv[])
{
    QVector<QByteArray> storage;
    storage.reserve(argc + 2);
    for (int i = 0; i < argc; ++i)
        storage.append(argv[i]);
    storage.append(QByteArrayLiteral("-maxwarnings"));
    storage.append(QByteArrayLiteral("10000000"));

    QVector<char *> arguments;
    arguments.reserve(storage.size());
    for (QByteArray &argument : storage)
        arguments.append(argument.data());

    int argumentCount = storage.size();
    QCoreApplication app(argumentCount, arguments.data());
    ConfigFileTest test;
    return QTest::qExec(&test, argumentCount, arguments.data());
}

#include "tst_config_file_test.moc"
