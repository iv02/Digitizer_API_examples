#pragma once

#include "iabstractsettingsmodel.h"

#include <QIdentityProxyModel>
#include <QList>

class QPersistentModelIndex;
class QTimer;

namespace client
{

enum BatchRoles
{
    BatchUpdateRole = Qt::UserRole + 100,
    BatchUpdateCompleteRole
};

class SettingDependencyController;
class FirmwareSettingsTableModel;

class SettingsStateProxyModel : public QIdentityProxyModel, public IAbstractSettingsModel
{
    Q_OBJECT

  signals:
    void validationFinished(ItemState state, IAbstractSettingsModel *model);
    void dependencyValidationFailed(const QString &message);

  public:
    explicit SettingsStateProxyModel(QObject *parent);
    ~SettingsStateProxyModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void commitSettings() override;
    void resetSettings() override;
    QAbstractItemModel *model() override;
    void setSourceModel(FirmwareSettingsTableModel *model);
    [[nodiscard]] bool hasSettingsChanges() const override;

  public slots:
    void setSettingsSchema(const QJsonObject &schema, int channelsNumber) override;
    void setSettingsValues(const QJsonObject &values) override;
    QVariantList getSettings() const override;
    QJsonObject getCurrentSettingsObject() override;

  private:
    void stateCheck();
    QString tableTag() const noexcept override;

  private:
    QTimer *m_validationTimer{nullptr};

    void validateBatchUpdates();

    bool m_isBatchUpdate = false;
    QList<QPersistentModelIndex> m_batchUpdatedIndices;

    SettingDependencyController *m_stateController{nullptr};
    FirmwareSettingsTableModel *m_model{nullptr};
    std::vector<std::vector<ItemState>> m_itemStates;
    std::vector<std::vector<ItemState>> m_itemStatesOld;
};

} // namespace client
