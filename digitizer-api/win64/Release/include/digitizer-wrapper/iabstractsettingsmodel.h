#pragma once

#include "settingitem.h"

#include <QAbstractTableModel>
#include <QJsonObject>

#include <memory>
#include <vector>

namespace client
{
class FwSettingsValidator;

enum SettingRole
{
    UiTypeRole = Qt::UserRole + 1,
    SettingNameRole,
    SettingDescriptionRole,
    IsGeneralRole,
    ComboBoxValuesRole,
    MinValueRole,
    MaxValueRole,
    StepRole,
    DecimalsRole,
    ItemStateRole,
    OldValueRole,
    IsVisibleRole,
    IsReadOnlyRole,
    DefaultRole
};

enum class ItemState
{
    Normal = 0,
    Modified,
    Invalid,
    Unknown
};

enum class ControlState
{
    Active = 0,
    Passive,
    SwitchOff
};

class IAbstractSettingsModel
{
  public:
    IAbstractSettingsModel(const QString &tableTag = QString());
    virtual ~IAbstractSettingsModel();

    virtual void setSettingsSchema(const QJsonObject &schema, int channelsNumber) = 0;
    virtual void setSettingsValues(const QJsonObject &values) = 0;
    [[nodiscard]] virtual QVariantList getSettings() const = 0;
    virtual QJsonObject getCurrentSettingsObject() = 0;
    virtual void commitSettings() = 0;
    virtual void resetSettings() = 0;
    virtual QAbstractItemModel *model() = 0;

    SettingItem::UiType uiTypeFromJson(const QJsonObject &object) noexcept;

    void fillComboBoxSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillToggleSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillIntLineEditSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillFloatLineEditSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillIntSpinBoxSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillFloatSpinBoxSettingItem(SettingItem &item, const QJsonObject &object) noexcept;
    void fillUnknownSettingItem(SettingItem &item, const QJsonObject &object) noexcept;

    void setIntegerRangeAndDefault(SettingItem &item, const QJsonObject &object) noexcept;
    void setNumberRangeAndDefault(SettingItem &item, const QJsonObject &object) noexcept;

    static void roundValueToStep(double &value, double step) noexcept;
    static void addVariantToJsonObject(QJsonObject &object, const QString &name, const QVariant &value) noexcept;

  protected:
    virtual QString tableTag() const noexcept = 0;

    static bool isInt(const QJsonValue &v) noexcept;
    QVariantMap parseOneOfValues(const SettingItem &item, const QJsonObject &object) const noexcept;
    int resolveComboBoxDefault(const SettingItem &item, const QJsonObject &object, const QVariantMap &oneOf) noexcept;
    void testComboBoxEmpty(const SettingItem &item, const QJsonObject &object) const noexcept;
    void testStepNull(const SettingItem &item, const QJsonObject &object, const bool &isDouble) const noexcept;
    std::vector<int> validateAndFixOrderNumbers(const QJsonObject &properties) noexcept;

    template <typename... Args> void warning(QString format, Args &&...args) const noexcept
    {
        if constexpr (sizeof...(args) > 0)
            ((format = format.arg(std::forward<Args>(args))), ...);

        qWarning().noquote() << tableTag() << format;
    }

    template <typename... Args> void testFields(const client::SettingItem &item, const QJsonObject &object, const Args &...args) const noexcept
    {
        static_assert(sizeof...(Args) > 0, "At least one field must be specified");

        QStringList missingFields;
        ((object[args].isUndefined() ? missingFields.append(QString(args)) : void()), ...);

        if (!missingFields.isEmpty())
            warning("Missing required fields '%1' of setting item '%2'", missingFields.join(", "), item.name);
    }

  protected:
    const QString m_tableTag;
    std::unique_ptr<FwSettingsValidator> m_settingsValidator;
};

} // namespace client
