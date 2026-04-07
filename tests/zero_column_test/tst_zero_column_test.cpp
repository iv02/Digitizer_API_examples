#include <QtTest>

#include "digitizerinteractor.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QThread>
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

static bool variantSettingDifferent(const QVariant &a, const QVariant &b)
{
    return !variantSettingEqual(a, b);
}

static void pushCandidate(std::vector<QVariant> &out, const QVariant &baseline, const QVariant &q)
{
    if (!q.isValid())
        return;
    if (variantSettingEqual(q, baseline))
        return;
    for (const QVariant &e : out)
    {
        if (variantSettingEqual(e, q))
            return;
    }
    out.push_back(q);
}

static QVariant lookupSchemaStep(const QJsonDocument &doc, const QString &fwTypeName, const QString &settingKey)
{
    if (!doc.isObject())
        return {};

    const QJsonObject tab = doc.object().value(QStringLiteral("properties")).toObject().value(fwTypeName).toObject();
    if (tab.isEmpty())
        return {};

    if (fwTypeName == QStringLiteral("Device"))
    {
        const QJsonObject prop = tab.value(QStringLiteral("properties")).toObject().value(settingKey).toObject();
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

static void addStepDisplacementCandidates(std::vector<QVariant> &c, const QVariant &baseline, const QVariant &schemaStep)
{
    if (!schemaStep.isValid() || schemaStep.isNull())
        return;

    const int bt = baseline.typeId();
    const bool isIntLike
        = (bt == QMetaType::Int || bt == QMetaType::LongLong || bt == QMetaType::UInt || bt == QMetaType::ULongLong);

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
    if (isFloatLike || (baseline.canConvert<double>() && baseline.metaType().id() != QMetaType::QString))
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

static std::vector<QVariant> perturbationCandidates(const QVariant &v, const QVariant &schemaStep)
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
        constexpr double steps[] = {1e-4, -1e-4, 1e-3, -1e-3, 1e-2, -1e-2, 0.05, -0.05, 0.1, -0.1};
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
        constexpr double steps[] = {1e-4, -1e-4, 1e-3, -1e-3, 1e-2, -1e-2, 0.05, -0.05, 0.1, -0.1};
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

struct PropagationCase
{
    QString fwTypeName;
    QString name;
    QVariant baseline0;
    std::vector<QVariant> baselinePerChannel;
    QVariant new0;
};

static bool tryFindPropagationCase(DigitizerInteractor &interactor,
                                  int64_t deviceId,
                                  const QJsonDocument &schemaDoc,
                                  int channelCount,
                                  const QString &dconfPath,
                                  PropagationCase &out)
{
    const QStringList fwNames = interactor.fwTypeNameList(deviceId);
    for (const QString &fw : fwNames)
    {
        if (fw == QStringLiteral("Device"))
            continue;

        const QStringList names = interactor.fwSettingList(deviceId, fw);
        for (const QString &settingName : names)
        {
            const QVariant baseline0 = interactor.getSetting(deviceId, fw, settingName, 0);
            if (!baseline0.isValid())
                continue;

            std::vector<QVariant> baselinePerChannel;
            baselinePerChannel.reserve(channelCount);
            bool okAll = true;
            for (int ch = 1; ch <= channelCount; ++ch)
            {
                const QVariant v = interactor.getSetting(deviceId, fw, settingName, ch);
                if (!v.isValid())
                {
                    okAll = false;
                    break;
                }
                baselinePerChannel.push_back(v);
            }
            if (!okAll)
                continue;

            const QVariant schemaStep = lookupSchemaStep(schemaDoc, fw, settingName);
            const std::vector<QVariant> candidates = perturbationCandidates(baseline0, schemaStep);
            if (candidates.empty())
                continue;

            for (const QVariant &new0 : candidates)
            {
                if (!interactor.setSetting(deviceId, fw, settingName, 0, new0))
                    continue;
                if (!interactor.uploadSettings(deviceId))
                {
                    (void)interactor.applyConfigurationFile(deviceId, dconfPath);
                    (void)interactor.downloadSettings(deviceId);
                    continue;
                }

                if (!interactor.downloadSettings(deviceId))
                {
                    (void)interactor.applyConfigurationFile(deviceId, dconfPath);
                    (void)interactor.downloadSettings(deviceId);
                    continue;
                }

                bool allMatchNew = true;
                bool allDifferentFromOld = true;
                for (int ch = 1; ch <= channelCount; ++ch)
                {
                    const QVariant v = interactor.getSetting(deviceId, fw, settingName, ch);
                    if (!v.isValid())
                    {
                        allMatchNew = false;
                        allDifferentFromOld = false;
                        break;
                    }
                    if (!variantSettingEqual(v, new0))
                        allMatchNew = false;
                    if (!variantSettingDifferent(v, baselinePerChannel[static_cast<size_t>(ch - 1)]))
                        allDifferentFromOld = false;
                }

                (void)interactor.applyConfigurationFile(deviceId, dconfPath);
                (void)interactor.downloadSettings(deviceId);

                if (!allMatchNew || !allDifferentFromOld)
                    continue;

                out.fwTypeName = fw;
                out.name = settingName;
                out.baseline0 = baseline0;
                out.baselinePerChannel = std::move(baselinePerChannel);
                out.new0 = new0;
                return true;
            }
        }
    }
    return false;
}

}

class ZeroColumnTest : public QObject
{
    Q_OBJECT

  private slots:
    void zero_column_propagates_to_channels();
};

void ZeroColumnTest::zero_column_propagates_to_channels()
{
    DigitizerInteractor interactor;

    QVERIFY2(waitForAnyDevice(interactor, kDiscoveryTimeoutMs),
             "No device discovered within timeout — connect a device and rerun.");

    const int64_t deviceId = interactor.devices().firstKey();
    QVERIFY2(interactor.connectDevice(deviceId), "connectDevice failed.");

    QVERIFY2(waitForSettingsCacheReady(interactor, deviceId, kSchemaWaitMs),
             "Firmware schema/settings not available within 1 s after connect (download/cache).");

    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed before exporting .dconf.");

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dconfPath = tmp.filePath(QStringLiteral("zero_column_test_baseline.dconf"));

    QVERIFY2(interactor.writeConfigurationFile(deviceId, dconfPath),
             "writeConfigurationFile failed — need cached settings.");

    QJsonParseError parseError;
    const QJsonDocument schemaDoc
        = QJsonDocument::fromJson(interactor.firmwareSettings(deviceId).first.toUtf8(), &parseError);
    (void)parseError;

    const int channelCount = static_cast<int>(interactor.getDeviceChannels(deviceId));
    QVERIFY2(channelCount >= 1, "Device reports zero channels.");

    PropagationCase c;
    QVERIFY2(tryFindPropagationCase(interactor, deviceId, schemaDoc, channelCount, dconfPath, c),
             "No setting found where column 0 successfully propagates to channels 1..N with a valid new value.");

    QVERIFY2(interactor.setSetting(deviceId, c.fwTypeName, c.name, 0, c.new0), "setSetting col 0 failed.");
    QVERIFY2(interactor.uploadSettings(deviceId), "uploadSettings failed after setting col 0.");
    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed after upload.");

    for (int ch = 1; ch <= channelCount; ++ch)
    {
        const QVariant v = interactor.getSetting(deviceId, c.fwTypeName, c.name, ch);
        QVERIFY2(v.isValid(), qPrintable(QStringLiteral("getSetting invalid: %1 / %2 col %3")
                                             .arg(c.fwTypeName, c.name)
                                             .arg(ch)));
        QVERIFY2(variantSettingEqual(v, c.new0),
                 qPrintable(QStringLiteral("col 0 did not propagate: %1 / %2 col %3")
                                .arg(c.fwTypeName, c.name)
                                .arg(ch)));
        QVERIFY2(variantSettingDifferent(v, c.baselinePerChannel[static_cast<size_t>(ch - 1)]),
                 qPrintable(QStringLiteral("channel did not change vs baseline: %1 / %2 col %3")
                                .arg(c.fwTypeName, c.name)
                                .arg(ch)));
    }

    const ConfigurationFileResult applied = interactor.applyConfigurationFile(deviceId, dconfPath);
    QVERIFY2(applied.status == ConfigurationFileStatus::Applied,
             qPrintable(QStringLiteral("applyConfigurationFile: %1").arg(applied.message)));

    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed after apply .dconf.");

    for (int ch = 1; ch <= channelCount; ++ch)
    {
        const QVariant v = interactor.getSetting(deviceId, c.fwTypeName, c.name, ch);
        QVERIFY2(v.isValid(), qPrintable(QStringLiteral("getSetting invalid after restore: %1 / %2 col %3")
                                             .arg(c.fwTypeName, c.name)
                                             .arg(ch)));
        QVERIFY2(variantSettingEqual(v, c.baselinePerChannel[static_cast<size_t>(ch - 1)]),
                 qPrintable(QStringLiteral("restore mismatch: %1 / %2 col %3")
                                .arg(c.fwTypeName, c.name)
                                .arg(ch)));
    }

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
    ZeroColumnTest test;
    return QTest::qExec(&test, ac, av.data());
}

#include "tst_zero_column_test.moc"

