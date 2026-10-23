#include <QtTest>

#include "firmwaresettingstablemodel.h"

#include <QCoreApplication>
#include <QJsonObject>
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
    const QJsonObject properties{{"threshold", integerSetting("Threshold", 0, 10)}};
    return QJsonObject{{"patternProperties", QJsonObject{{"^channel[0-9]{2}$", QJsonObject{{"properties", properties}}}}}};
}

} // namespace

class ZeroColumnTest : public QObject
{
    Q_OBJECT

  private slots:
    void zero_column_propagates_to_channels();
};

void ZeroColumnTest::zero_column_propagates_to_channels()
{
    client::FirmwareSettingsTableModel model;
    model.setSettingsSchema(testSchema(), 3);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.columnCount(), 5); // title, general and three channels

    const QModelIndex generalIndex = model.index(0, 1);
    QVERIFY(generalIndex.isValid());
    QVERIFY(model.setData(generalIndex, 42, Qt::EditRole));

    for (int column = 1; column < model.columnCount(); ++column)
        QCOMPARE(model.data(model.index(0, column), Qt::DisplayRole).toInt(), 42);

    QVERIFY(model.setData(model.index(0, 3), 7, Qt::EditRole));
    QCOMPARE(model.data(model.index(0, 2), Qt::DisplayRole).toInt(), 42);
    QCOMPARE(model.data(model.index(0, 3), Qt::DisplayRole).toInt(), 7);
    QCOMPARE(model.data(model.index(0, 4), Qt::DisplayRole).toInt(), 42);
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
