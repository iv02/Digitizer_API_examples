#pragma once

#include <QMap>
#include <QObject>

class QAbstractItemModel;

namespace client
{

class SettingDependency : public QObject
{
    Q_OBJECT
  public:
    enum class ComparisonOperator
    {
        Greater = 0,
        GreaterOrEqual,
        Equal,
        NotEqual,
        Less,
        LessOrEqual,
        Empty
    };

  public:
    SettingDependency() = default;
    SettingDependency(const QMap<QString, QVariant> &dependencyParameters, std::pair<int, int> orderAndChannel, QAbstractItemModel *model);
    SettingDependency(SettingDependency &&dependency) noexcept;
    SettingDependency(const SettingDependency &dependency);
    bool isAcceptable();
    QString reason() const;

  private:
    static inline QMap<QString, ComparisonOperator> m_comparisonMap = {
        {">", ComparisonOperator::Greater}, {">=", ComparisonOperator::GreaterOrEqual}, {"==", ComparisonOperator::Equal}, {"!=", ComparisonOperator::NotEqual},
        {"<", ComparisonOperator::Less},    {"<=", ComparisonOperator::LessOrEqual},    {"", ComparisonOperator::Empty}};

  private:
    std::pair<int, int> m_orderAndChannel{};
    double m_lastResult{};
    QAbstractItemModel *m_model{nullptr};
    QStringList m_settingsNames{};
    ComparisonOperator m_operator{ComparisonOperator::Empty};
    QString m_expression{};
};

} // namespace client
