#pragma once

#include "iabstractsettingsmodel.h"
#include "settingitem.h"

#include <QAbstractTableModel>

class QJsonObject;

namespace client
{

class DeviceSettingsTableModel : public QAbstractTableModel, public IAbstractSettingsModel
{
    Q_OBJECT
  signals:
    void validationFinished(ItemState state, IAbstractSettingsModel *model);

  public:
    enum Sections
    {
        Title = 0,
        Value,
        Count
    };

  public:
    explicit DeviceSettingsTableModel(QObject *parent = nullptr, const QString &tableTag = QString());
    ~DeviceSettingsTableModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void commitSettings() override;
    void resetSettings() override;
    QAbstractItemModel *model() override;

  public slots:
    void setSettingsSchema(const QJsonObject &schema, int channelsNumber) override;
    void setSettingsValues(const QJsonObject &values) override;
    QVariantList getSettings() const override;
    QJsonObject getCurrentSettingsObject() override;
    [[nodiscard]] bool hasChanges() const override;

  private:
    QString tableTag() const noexcept override;

  private:
    std::vector<SettingItem> m_schemaData;                       // index - order
    std::vector<std::pair<ItemState, QVariant>> m_valuesData;    //  index - order | ItemState - state, QVariant - value
    std::vector<std::pair<ItemState, QVariant>> m_valuesDataOld; //  index - order | ItemState - state, QVariant - value
};

} // namespace client
