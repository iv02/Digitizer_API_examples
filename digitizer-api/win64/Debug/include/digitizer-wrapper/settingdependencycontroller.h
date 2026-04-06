#pragma once

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPersistentModelIndex>
#include <vector>

class QAbstractItemModel;

namespace client
{

class SettingDependency;

class SettingDependencyController : public QObject
{
    Q_OBJECT

  signals:
    void itemValidationFailed(const QMap<QPersistentModelIndex, QString> &error);
    void itemValidationSuccess(const QModelIndex &index) const;

  public:
    explicit SettingDependencyController(QAbstractItemModel *model);

  public slots:
    void onModelChanged(const QJsonObject &schema, int channelsNumber);
    void onValuesChanged(const QModelIndex &itemIndex);
    void onValuesChanged();

  private:
    QMap<QPersistentModelIndex, QString> validateChannelData(int column);

  private:
    QAbstractItemModel *m_model{nullptr};
    std::vector<std::vector<std::vector<SettingDependency>>> m_settingDependency;
};

} // namespace client
