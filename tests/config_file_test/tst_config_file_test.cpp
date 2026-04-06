#include <QtTest>

#include "digitizerinteractor.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QTemporaryDir>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

using namespace digi;

namespace
{

constexpr int kDiscoveryTimeoutMs = 60000;
constexpr int kSchemaWaitMs = 1000;

bool waitForAnyDevice(DigitizerInteractor &interactor, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs)
    {
        if (!interactor.devices().isEmpty())
            return true;
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }
    return !interactor.devices().isEmpty();
}

bool waitForSettingsCacheReady(DigitizerInteractor &interactor, int64_t deviceId, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs)
    {
        (void)interactor.downloadSettings(deviceId);
        const auto metaValues = interactor.firmwareSettings(deviceId);
        if (!metaValues.first.isEmpty() && !metaValues.second.isEmpty())
            return true;
        QCoreApplication::processEvents();
        QThread::msleep(20);
    }
    const auto metaValues = interactor.firmwareSettings(deviceId);
    return !metaValues.first.isEmpty() && !metaValues.second.isEmpty();
}

struct SettingSlot
{
    QString fwTypeName;
    QString name;
    int column;
    QVariant baseline;
    QVariant perturbed;
};

static bool variantSameNumber(const QVariant &a, const QVariant &b)
{
    if (a.typeId() == QMetaType::Double || a.typeId() == QMetaType::Float)
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    if (b.typeId() == QMetaType::Double || b.typeId() == QMetaType::Float)
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    return a == b;
}

static std::optional<bool> variantAsBool01(const QVariant &v)
{
    switch (v.typeId())
    {
    case QMetaType::Bool:
        return v.toBool();
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong: {
        const qlonglong n = v.toLongLong();
        if (n == 0)
            return false;
        if (n == 1)
            return true;
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

static bool variantSettingEqual(const QVariant &device, const QVariant &expected)
{
    if (device == expected)
        return true;
    if (variantSameNumber(device, expected))
        return true;
    const auto bd = variantAsBool01(device);
    const auto be = variantAsBool01(expected);
    if (bd.has_value() && be.has_value())
        return bd.value() == be.value();
    return false;
}

static void pushCandidate(std::vector<QVariant> &out, const QVariant &baseline, const QVariant &q)
{
    if (!q.isValid())
        return;
    if (variantSameNumber(q, baseline))
        return;
    for (const QVariant &e : out)
    {
        if ((e.typeId() == QMetaType::Double || e.typeId() == QMetaType::Float)
            && (q.typeId() == QMetaType::Double || q.typeId() == QMetaType::Float))
        {
            if (qFuzzyCompare(e.toDouble(), q.toDouble()))
                return;
        }
        else if (e == q)
            return;
    }
    out.push_back(q);
}

static QVariant lookupSchemaStep(const QJsonDocument &doc, const QString &fwTypeName,
                                 const QString &settingKey)
{
    if (!doc.isObject())
        return {};

    const QJsonObject tab = doc.object().value(QStringLiteral("properties")).toObject().value(fwTypeName).toObject();
    if (tab.isEmpty())
        return {};

    if (fwTypeName == QStringLiteral("Device"))
    {
        const QJsonObject prop
            = tab.value(QStringLiteral("properties")).toObject().value(settingKey).toObject();
        if (prop.contains(QStringLiteral("step")))
            return prop.value(QStringLiteral("step")).toVariant();
        return {};
    }

    const QJsonObject patternProps = tab.value(QStringLiteral("patternProperties")).toObject();
    for (auto it = patternProps.constBegin(); it != patternProps.constEnd(); ++it)
    {
        const QJsonObject props = it.value().toObject().value(QStringLiteral("properties")).toObject();
        const QJsonObject item = props.value(settingKey).toObject();
        if (!item.isEmpty() && item.contains(QStringLiteral("step")))
            return item.value(QStringLiteral("step")).toVariant();
    }
    return {};
}

static void addStepDisplacementCandidates(std::vector<QVariant> &c, const QVariant &baseline,
                                        const QVariant &schemaStep)
{
    if (!schemaStep.isValid() || schemaStep.isNull())
        return;

    const int bt = baseline.typeId();
    const bool isIntLike = (bt == QMetaType::Int || bt == QMetaType::LongLong || bt == QMetaType::UInt
                            || bt == QMetaType::ULongLong);

    if (isIntLike || (baseline.canConvert<qlonglong>() && baseline.metaType().id() != QMetaType::QString
                      && baseline.metaType().id() != QMetaType::Double && baseline.metaType().id() != QMetaType::Float))
    {
        bool ok = false;
        int stepI = schemaStep.toInt(&ok);
        if (!ok || stepI < 1)
        {
            const double sd = schemaStep.toDouble(&ok);
            if (!ok || sd < 1.0 || !std::isfinite(sd))
                return;
            stepI = static_cast<int>(std::llround(sd));
            if (stepI < 1)
                return;
        }
        const qlonglong x = baseline.toLongLong();
        pushCandidate(c, baseline, QVariant::fromValue(x + stepI));
        pushCandidate(c, baseline, QVariant::fromValue(x - stepI));
        return;
    }

    const bool isFloatLike = (bt == QMetaType::Double || bt == QMetaType::Float);
    if (isFloatLike
        || (baseline.canConvert<double>() && baseline.metaType().id() != QMetaType::QString))
    {
        bool ok = false;
        const double sd = schemaStep.toDouble(&ok);
        if (!ok || !(sd > 0.0) || !std::isfinite(sd))
            return;
        const double x = baseline.toDouble();
        if (!std::isfinite(x))
            return;
        const double yp = x + sd;
        const double ym = x - sd;
        if (std::isfinite(yp))
            pushCandidate(c, baseline, QVariant(yp));
        if (std::isfinite(ym))
            pushCandidate(c, baseline, QVariant(ym));
    }
}

std::vector<QVariant> perturbationCandidates(const QVariant &v, const QVariant &schemaStep)
{
    std::vector<QVariant> c;

    switch (v.typeId())
    {
    case QMetaType::Bool:
        pushCandidate(c, v, QVariant(!v.toBool()));
        return c;
    case QMetaType::QString:
        return c;
    case QMetaType::Double:
    case QMetaType::Float: {
        addStepDisplacementCandidates(c, v, schemaStep);
        const double x = v.toDouble();
        constexpr double steps[] = {
            1e-4, -1e-4, 1e-3, -1e-3, 1e-2, -1e-2, 0.05, -0.05, 0.1, -0.1,
        };
        for (double d : steps)
        {
            const double y = x + d;
            if (!std::isfinite(y))
                continue;
            pushCandidate(c, v, QVariant(y));
        }
        if (std::abs(x) >= 8.0)
        {
            const double yp = x + 1.0;
            const double ym = x - 1.0;
            if (std::isfinite(yp))
                pushCandidate(c, v, QVariant(yp));
            if (std::isfinite(ym))
                pushCandidate(c, v, QVariant(ym));
        }
        return c;
    }
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        addStepDisplacementCandidates(c, v, schemaStep);
        pushCandidate(c, v, QVariant::fromValue(v.toLongLong() + 1));
        pushCandidate(c, v, QVariant::fromValue(v.toLongLong() - 1));
        return c;
    default:
        break;
    }

    if (v.metaType().id() == QMetaType::QString || v.typeId() == QMetaType::UnknownType)
        return c;

    if (v.canConvert<double>() && v.metaType().id() != QMetaType::QString)
    {
        addStepDisplacementCandidates(c, v, schemaStep);
        const double x = v.toDouble();
        constexpr double steps[] = {
            1e-4, -1e-4, 1e-3, -1e-3, 1e-2, -1e-2, 0.05, -0.05, 0.1, -0.1,
        };
        for (double d : steps)
        {
            const double y = x + d;
            if (!std::isfinite(y))
                continue;
            pushCandidate(c, v, QVariant(y));
        }
        if (std::abs(x) >= 8.0)
        {
            if (std::isfinite(x + 1.0))
                pushCandidate(c, v, QVariant(x + 1.0));
            if (std::isfinite(x - 1.0))
                pushCandidate(c, v, QVariant(x - 1.0));
        }
        if (!c.empty())
            return c;
    }

    if (v.canConvert<qlonglong>())
    {
        addStepDisplacementCandidates(c, v, schemaStep);
        const qlonglong n = v.toLongLong();
        pushCandidate(c, v, QVariant::fromValue(n + 1));
        pushCandidate(c, v, QVariant::fromValue(n - 1));
    }

    return c;
}

int columnLimit(DigitizerInteractor &interactor, int64_t deviceId, const QString &fwTypeName)
{
    const uint16_t ch = interactor.getDeviceChannels(deviceId);
    if (fwTypeName == QStringLiteral("Device"))
        return 2;
    return std::max(16, static_cast<int>(ch) + 2);
}

std::vector<SettingSlot> collectPerturbableSlots(DigitizerInteractor &interactor, int64_t deviceId)
{
    std::vector<SettingSlot> out;
    QJsonParseError parseError;
    const QJsonDocument schemaDoc
        = QJsonDocument::fromJson(interactor.firmwareSettings(deviceId).first.toUtf8(), &parseError);
    (void)parseError;

    const QStringList fwNames = interactor.fwTypeNameList(deviceId);
    for (const QString &fw : fwNames)
    {
        const QStringList names = interactor.fwSettingList(deviceId, fw);
        const int maxCol = columnLimit(interactor, deviceId, fw);
        for (const QString &settingName : names)
        {
            const QVariant schemaStep = lookupSchemaStep(schemaDoc, fw, settingName);
            for (int col = 1; col < maxCol; ++col)
            {
                const QVariant cur = interactor.getSetting(deviceId, fw, settingName, col);
                if (!cur.isValid())
                    continue;
                const std::vector<QVariant> candidates = perturbationCandidates(cur, schemaStep);
                std::optional<QVariant> chosen;
                for (const QVariant &next : candidates)
                {
                    if (!interactor.setSetting(deviceId, fw, settingName, col, next))
                        continue;
                    if (!interactor.uploadSettings(deviceId))
                    {
                        (void)interactor.setSetting(deviceId, fw, settingName, col, cur);
                        (void)interactor.downloadSettings(deviceId);
                        continue;
                    }
                    if (!interactor.setSetting(deviceId, fw, settingName, col, cur))
                    {
                        (void)interactor.downloadSettings(deviceId);
                        continue;
                    }
                    if (!interactor.uploadSettings(deviceId))
                    {
                        (void)interactor.downloadSettings(deviceId);
                        continue;
                    }
                    (void)interactor.downloadSettings(deviceId);
                    chosen = next;
                    break;
                }
                if (!chosen.has_value())
                    continue;
                out.push_back(SettingSlot{fw, settingName, col, cur, chosen.value()});
            }
        }
    }
    return out;
}

}

class ConfigFileTest : public QObject
{
    Q_OBJECT

  private slots:
    void device_configuration_file_scenario();
};

void ConfigFileTest::device_configuration_file_scenario()
{
    // 1. Create an interactor and wait until at least one device appears (USB/network discovery).
    DigitizerInteractor interactor;

    QVERIFY2(waitForAnyDevice(interactor, kDiscoveryTimeoutMs),
             "No device discovered within timeout — connect a device and rerun.");

    // 2. Pick the first discovered device id for the rest of the scenario.
    const int64_t deviceId = interactor.devices().firstKey();

    // 3. Open a session with the device.
    QVERIFY2(interactor.connectDevice(deviceId), "connectDevice failed.");

    // 4. Ensure firmware JSON schema and current values are cached (short poll after connect).
    QVERIFY2(waitForSettingsCacheReady(interactor, deviceId, kSchemaWaitMs),
             "Firmware schema/settings not available within 1 s after connect (download/cache).");

    // 5. Refresh local settings from hardware before exporting a snapshot.
    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed before exporting .dconf.");

    // 6. Persist the current device configuration to a temporary .dconf baseline file.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dconfPath = tmp.filePath(QStringLiteral("config_file_test_baseline.dconf"));

    QVERIFY2(interactor.writeConfigurationFile(deviceId, dconfPath),
             "writeConfigurationFile failed — need cached settings.");

    // 7. For each firmware tab and setting key (column > 0 only), probe writable changes via
    //    set/upload/revert prechecks; collect slots with valid baseline vs perturbed values.
    std::vector<SettingSlot> perturbedSettings = collectPerturbableSlots(interactor, deviceId);
    QVERIFY2(!perturbedSettings.empty(), "No perturbable settings from fwTypeNameList/fwSettingList/getSetting.");

    // 8. Apply all perturbed values locally, then push the full set to the device in one upload.
    for (const SettingSlot &s : perturbedSettings)
    {
        QVERIFY2(interactor.setSetting(deviceId, s.fwTypeName, s.name, s.column, s.perturbed),
                 qPrintable(QStringLiteral("setSetting failed: %1 / %2 col %3")
                                .arg(s.fwTypeName, s.name)
                                .arg(s.column)));
    }

    QVERIFY2(interactor.uploadSettings(deviceId), "uploadSettings failed after bulk change.");

    // 9. Pull settings back and assert each slot matches its perturbed value on the device.
    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed after upload.");

    for (const SettingSlot &s : perturbedSettings)
    {
        const QVariant v = interactor.getSetting(deviceId, s.fwTypeName, s.name, s.column);
        QVERIFY2(v.isValid(), qPrintable(QStringLiteral("getSetting invalid: %1 / %2 col %3")
                                             .arg(s.fwTypeName, s.name)
                                             .arg(s.column)));
        QVERIFY2(variantSettingEqual(v, s.perturbed),
                 qPrintable(QStringLiteral("device value mismatch after upload: %1 / %2 col %3")
                                .arg(s.fwTypeName, s.name)
                                .arg(s.column)));
    }

    // 10. Restore configuration from the saved .dconf (should overwrite the perturbations).
    const ConfigurationFileResult applied = interactor.applyConfigurationFile(deviceId, dconfPath);
    QVERIFY2(applied.status == ConfigurationFileStatus::Applied,
             qPrintable(QStringLiteral("applyConfigurationFile: %1").arg(applied.message)));

    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed after apply .dconf.");

    // 11. Confirm every previously perturbed slot matches the original baseline again.
    for (const SettingSlot &s : perturbedSettings)
    {
        const QVariant v = interactor.getSetting(deviceId, s.fwTypeName, s.name, s.column);
        QVERIFY2(v.isValid(), qPrintable(QStringLiteral("getSetting invalid after apply: %1 / %2 col %3")
                                             .arg(s.fwTypeName, s.name)
                                            .arg(s.column)));
        QVERIFY2(variantSettingEqual(v, s.baseline),
                 qPrintable(QStringLiteral("after .dconf apply expected baseline: %1 / %2 col %3")
                                .arg(s.fwTypeName, s.name)
                                .arg(s.column)));
    }

    // 12. Tear down the connection.
    (void)interactor.disconnectDevice(deviceId);
}

int main(int argc, char *argv[])
{
    QVector<QByteArray> storage;
    storage.reserve(argc + 2);
    for (int i = 0; i < argc; ++i)
        storage.append(argv[i]);
    storage.append(QByteArrayLiteral("-maxwarnings"));
    storage.append(QByteArrayLiteral("10000000"));

    QVector<char *> av;
    av.reserve(storage.size());
    for (QByteArray &b : storage)
        av.append(b.data());

    int ac = storage.size();
    QCoreApplication app(ac, av.data());
    ConfigFileTest test;
    return QTest::qExec(&test, ac, av.data());
}

#include "tst_config_file_test.moc"
