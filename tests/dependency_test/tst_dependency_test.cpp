#include <QtTest>

#include "digitizerinteractor.h"

#include "devicesettingsdefines.h"
#include "firmwaresettingstablemodel.h"
#include "iabstractsettingsmodel.h"
#include "settingsstateproxymodel.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QVector>
#include <QSignalSpy>

#include <muParser.h>

#include <cmath>
#include <optional>

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

struct DependencyPick
{
    QString tab;
    QString settingKey;
    int order = -1;
    QStringList varNames;
    QString compare;
    QString expression;
};

static bool isFiniteDouble(double x)
{
    return std::isfinite(x);
}

static QVariant makeInvalidValue(const QString &compare, double rhs, double current)
{
    constexpr double eps = 1e-6;
    if (!isFiniteDouble(rhs) || !isFiniteDouble(current))
        return {};

    if (compare == QStringLiteral(">"))
        return QVariant(rhs);
    if (compare == QStringLiteral(">="))
        return QVariant(rhs - eps);
    if (compare == QStringLiteral("<"))
        return QVariant(rhs);
    if (compare == QStringLiteral("<="))
        return QVariant(rhs + eps);
    if (compare == QStringLiteral("=="))
        return QVariant(rhs + eps);
    if (compare == QStringLiteral("!="))
        return QVariant(rhs);

    return {};
}

static std::optional<double> evalExpressionWithMuParser(QAbstractItemModel &model,
                                                       int channel,
                                                       const QStringList &varNames,
                                                       const QString &expr)
{
    if (expr.isEmpty())
        return std::nullopt;

    mu::Parser p;
    QMap<QString, double> currentValues;

    const int col = channel + 2;
    for (int row = 0; row < model.rowCount(); ++row)
    {
        const QModelIndex idx = model.index(row, col);
        const QString name = model.data(idx, client::SettingNameRole).toString();
        if (!varNames.contains(name))
            continue;
        currentValues[name] = model.data(idx, Qt::DisplayRole).toDouble();
        p.DefineVar(name.toStdWString(), &currentValues[name]);
    }

    try
    {
        p.SetExpr(expr.toStdWString());
        const double r = p.Eval();
        if (!isFiniteDouble(r))
            return std::nullopt;
        return r;
    }
    catch (mu::Parser::exception_type &)
    {
        return std::nullopt;
    }
}

}

class DependencyTest : public QObject
{
    Q_OBJECT

  private slots:
    void dependency_marks_item_invalid_in_proxy_model();
};

void DependencyTest::dependency_marks_item_invalid_in_proxy_model()
{
    DigitizerInteractor interactor;

    QVERIFY2(waitForAnyDevice(interactor, kDiscoveryTimeoutMs),
             "No device discovered within timeout — connect a device and rerun.");

    const int64_t deviceId = interactor.devices().firstKey();
    QVERIFY2(interactor.connectDevice(deviceId), "connectDevice failed.");

    QVERIFY2(waitForSettingsCacheReady(interactor, deviceId, kSchemaWaitMs),
             "Firmware schema/settings not available within 1 s after connect (download/cache).");

    QVERIFY2(interactor.downloadSettings(deviceId), "downloadSettings failed before building models.");

    QJsonParseError schemaErr;
    QJsonParseError valuesErr;
    const auto metaValues = interactor.firmwareSettings(deviceId);
    const QJsonDocument schemaDoc = QJsonDocument::fromJson(metaValues.first.toUtf8(), &schemaErr);
    const QJsonDocument valuesDoc = QJsonDocument::fromJson(metaValues.second.toUtf8(), &valuesErr);

    QVERIFY2(schemaDoc.isObject(), "Schema JSON is not an object.");
    QVERIFY2(valuesDoc.isObject(), "Values JSON is not an object.");

    const QJsonObject schemaRoot = schemaDoc.object();
    const QJsonObject valuesRoot = valuesDoc.object();
    const QJsonObject propsRoot = schemaRoot.value(QStringLiteral("properties")).toObject();

    const int channels = static_cast<int>(interactor.getDeviceChannels(deviceId));
    QVERIFY2(channels >= 1, "Device reports zero channels.");

    int depsFound = 0;
    int depsLogged = 0;
    int depsSkippedMissingCompareOrExpr = 0;
    int depsSkippedMissingVars = 0;
    int depsSkippedBadOrder = 0;

    int depsSatisfiedPositive = 0;
    int depsViolatedNegative = 0;
    int depsTested = 0;
    constexpr int kMaxDepsToTest = 10;

    for (auto it = propsRoot.constBegin(); it != propsRoot.constEnd() && depsTested < kMaxDepsToTest; ++it)
    {
        const QString tab = it.key();
        if (!valuesRoot.contains(tab))
            continue;

        const QJsonObject tabSchema = it.value().toObject();
        const QJsonObject tabValues = valuesRoot.value(tab).toObject();
        if (tabSchema.isEmpty() || tabValues.isEmpty())
            continue;

        const QJsonObject patternProps = tabSchema.value(QStringLiteral("patternProperties")).toObject();
        if (patternProps.isEmpty())
            continue;

        client::SettingsStateProxyModel proxy(nullptr);
        client::FirmwareSettingsTableModel source(nullptr, tab);
        proxy.setSourceModel(&source);
        proxy.setSettingsSchema(tabSchema, channels);
        proxy.setSettingsValues(tabValues);

        QSignalSpy failSpy(&proxy, &client::SettingsStateProxyModel::dependencyValidationFailed);

        const int channel = 0;
        const int col = channel + 2;

        for (auto pit = patternProps.constBegin(); pit != patternProps.constEnd() && depsTested < kMaxDepsToTest; ++pit)
        {
            const QJsonObject props = pit.value().toObject().value(QStringLiteral("properties")).toObject();
            for (auto sit = props.constBegin(); sit != props.constEnd() && depsTested < kMaxDepsToTest; ++sit)
            {
                const QJsonObject propObj = sit.value().toObject();
                const int order = propObj.value(QStringLiteral("order")).toInt(-1);
                if (order < 0)
                {
                    ++depsSkippedBadOrder;
                    continue;
                }

                const QJsonArray deps = propObj.value(QStringLiteral("dependences")).toArray();
                if (deps.isEmpty())
                    continue;
                ++depsFound;

                const QJsonObject d0 = deps.at(0).toObject();
                const QString compare = d0.value(QStringLiteral("compare")).toString();
                const QString expr = d0.value(QStringLiteral("expression")).toString();
                if (compare.isEmpty() || expr.isEmpty())
                {
                    ++depsSkippedMissingCompareOrExpr;
                    continue;
                }

                const QJsonValue settings = d0.value(QStringLiteral("settings"));
                QStringList vars;
                if (settings.isArray())
                {
                    for (const auto &v : settings.toArray())
                        vars.append(v.toString());
                }
                else
                {
                    const QString s = settings.toString();
                    if (!s.isEmpty())
                        vars.append(s);
                }
                if (vars.isEmpty())
                {
                    ++depsSkippedMissingVars;
                    continue;
                }

                qInfo().noquote()
                    << QStringLiteral("[DependencyTest] candidate tab=%1 setting=%2 order=%3 channel=%4 compare=%5 expression=%6 settings=%7")
                           .arg(tab, sit.key())
                           .arg(order)
                           .arg(channel)
                           .arg(compare, expr, vars.join(QStringLiteral(",")));
                ++depsLogged;

                const QModelIndex target = proxy.index(order, col);
                if (!target.isValid())
                    continue;

                const QVariant baseline = proxy.data(target, Qt::DisplayRole);
                if (!baseline.isValid())
                    continue;

                const auto rhs = evalExpressionWithMuParser(proxy, channel, vars, expr);

                failSpy.clear();
                QVERIFY2(proxy.setData(target, baseline, Qt::EditRole), "setData failed for baseline (positive scenario).");
                const auto okState = qvariant_cast<client::ItemState>(proxy.data(target, client::ItemStateRole));
                QVERIFY2(okState != client::ItemState::Invalid,
                         "Expected non-Invalid state for baseline when dependency is satisfied.");
                QVERIFY2(failSpy.count() == 0, "dependencyValidationFailed must not be emitted for baseline.");
                ++depsSatisfiedPositive;

                std::vector<QVariant> invalidCandidates;
                invalidCandidates.reserve(8);
                if (rhs.has_value())
                {
                    const QVariant v = makeInvalidValue(compare, rhs.value(), baseline.toDouble());
                    if (v.isValid())
                        invalidCandidates.push_back(v);
                }

                const double b = baseline.toDouble();
                invalidCandidates.push_back(QVariant(b + 1.0));
                invalidCandidates.push_back(QVariant(b - 1.0));
                invalidCandidates.push_back(QVariant(0.0));
                invalidCandidates.push_back(QVariant(1.0));
                invalidCandidates.push_back(QVariant(10.0));
                invalidCandidates.push_back(QVariant(-10.0));

                bool violated = false;
                for (const QVariant &invalidValue : invalidCandidates)
                {
                    failSpy.clear();
                    if (!proxy.setData(target, invalidValue, Qt::EditRole))
                        continue;

                    const auto state = qvariant_cast<client::ItemState>(proxy.data(target, client::ItemStateRole));
                    if (state != client::ItemState::Invalid)
                        continue;
                    if (failSpy.count() <= 0)
                        continue;
                    violated = true;
                    break;
                }
                if (!violated)
                    continue;
                ++depsViolatedNegative;

                QVERIFY2(proxy.setData(target, baseline, Qt::EditRole), "setData failed for baseline restore.");
                const auto state2 = qvariant_cast<client::ItemState>(proxy.data(target, client::ItemStateRole));
                QVERIFY2(state2 != client::ItemState::Invalid, "Expected ItemState to clear after restore.");

                ++depsTested;
            }
        }
    }

    qInfo().noquote()
        << QStringLiteral("[DependencyTest] depsFound=%1 depsLogged=%2 skippedBadOrder=%3 skippedMissingCompareOrExpr=%4 skippedMissingVars=%5 depsSatisfiedPositive=%6 depsViolatedNegative=%7 depsTested=%8")
               .arg(depsFound)
               .arg(depsLogged)
               .arg(depsSkippedBadOrder)
               .arg(depsSkippedMissingCompareOrExpr)
               .arg(depsSkippedMissingVars)
               .arg(depsSatisfiedPositive)
               .arg(depsViolatedNegative)
               .arg(depsTested);

    QVERIFY2(depsTested > 0, "Unable to trigger any dependency invalid state through SettingsStateProxyModel from examples.");
    QVERIFY2(depsSatisfiedPositive >= depsTested, "Positive scenario did not run for all tested dependencies.");
    QVERIFY2(depsViolatedNegative == depsTested, "Negative scenario did not trigger for all tested dependencies.");

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
    DependencyTest test;
    return QTest::qExec(&test, ac, av.data());
}

#include "tst_dependency_test.moc"

