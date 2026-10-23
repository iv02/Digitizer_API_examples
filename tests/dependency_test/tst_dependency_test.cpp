#include <QtTest>

#include "firmwaresettingstablemodel.h"
#include "settingsstateproxymodel.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QVector>

namespace
{

QJsonObject integerSetting(const QString &description, int order, int defaultValue)
{
    return QJsonObject{{"ui-type", "lineedit"},
                       {"type", "integer"},
                       {"base", "int32_t"},
                       {"order", order},
                       {"default", defaultValue},
                       {"minimum", 0},
                       {"maximum", 100},
                       {"description", description}};
}

QJsonObject testSchema()
{
    QJsonObject threshold = integerSetting("Threshold", 1, 15);
    threshold.insert("dependences", QJsonArray{QJsonObject{{"settings", "limit"},
                                                           {"compare", ">"},
                                                           {"expression", "limit"}}});

    const QJsonObject properties{{"limit", integerSetting("Limit", 0, 10)}, {"threshold", threshold}};
    return QJsonObject{{"patternProperties", QJsonObject{{"^channel[0-9]{2}$", QJsonObject{{"properties", properties}}}}}};
}

QJsonObject testValues()
{
    return QJsonObject{{"channel00", QJsonObject{{"limit", 10}, {"threshold", 15}}}};
}

} // namespace

class DependencyTest : public QObject
{
    Q_OBJECT

  private slots:
    void dependency_marks_item_invalid_in_proxy_model();
};

void DependencyTest::dependency_marks_item_invalid_in_proxy_model()
{
    client::FirmwareSettingsTableModel source(nullptr, QStringLiteral("autonomous"));
    client::SettingsStateProxyModel proxy(nullptr);
    proxy.setSourceModel(&source);
    proxy.setSettingsSchema(testSchema(), 1);
    proxy.setSettingsValues(testValues());

    const QModelIndex thresholdIndex = proxy.index(1, 2); // threshold, channel 0
    QVERIFY(thresholdIndex.isValid());
    QCOMPARE(proxy.data(thresholdIndex, Qt::DisplayRole).toInt(), 15);
    QCOMPARE(qvariant_cast<client::ItemState>(proxy.data(thresholdIndex, client::ItemStateRole)),
             client::ItemState::Normal);

    QSignalSpy validationFailed(&proxy, &client::SettingsStateProxyModel::dependencyValidationFailed);

    QVERIFY(proxy.setData(thresholdIndex, 5, Qt::EditRole));
    QCOMPARE(proxy.data(thresholdIndex, Qt::DisplayRole).toInt(), 5);
    QCOMPARE(qvariant_cast<client::ItemState>(proxy.data(thresholdIndex, client::ItemStateRole)),
             client::ItemState::Invalid);
    QCOMPARE(validationFailed.count(), 1);

    QVERIFY(proxy.setData(thresholdIndex, 15, Qt::EditRole));
    QTRY_COMPARE(qvariant_cast<client::ItemState>(proxy.data(thresholdIndex, client::ItemStateRole)),
                 client::ItemState::Normal);
    QCOMPARE(validationFailed.count(), 1);
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
