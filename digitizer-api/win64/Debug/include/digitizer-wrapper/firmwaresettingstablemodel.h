#pragma once

#include "iabstractsettingsmodel.h"
#include "settingitem.h"

#include <QAbstractTableModel>

class QJsonObject;

namespace client
{
class FirmwareSettingsTableModel : public QAbstractTableModel, public IAbstractSettingsModel
{
    Q_OBJECT

  public:
    enum Sections
    {
        Title = 0,
        General
    };

  public:
    explicit FirmwareSettingsTableModel(QObject *parent = nullptr, const QString &tableTag = QString());
    ~FirmwareSettingsTableModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void commitSettings() override;
    void resetSettings() override;
    QAbstractItemModel *model() override;

    [[nodiscard]] QVariantList getSettings() const override;

  public slots:
    void setSettingsSchema(const QJsonObject &schema, int channelsNumber) override;
    void setSettingsValues(const QJsonObject &values) override;
    QJsonObject getCurrentSettingsObject() override;

  private:
    QString tableTag() const noexcept override;

  protected:
    std::vector<SettingItem> m_schemaData;              // index - order
    std::vector<std::vector<QVariant>> m_valuesData;    // index1 - channel | index2 - order | QVariant - value
    std::vector<std::vector<QVariant>> m_valuesDataOld; // index1 - channel | index2 - order | QVariant - value
};

} // namespace client
