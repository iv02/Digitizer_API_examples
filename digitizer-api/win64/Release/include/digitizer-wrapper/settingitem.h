#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <utility>
#include <vector>

namespace client
{

using ComboBoxValues = std::vector<std::pair<QString, QVariant>>;

[[nodiscard]] inline ComboBoxValues comboBoxValues(const QVariant &stored)
{
    return stored.value<ComboBoxValues>();
}

[[nodiscard]] inline QString comboBoxValueTitle(const ComboBoxValues &values, const QVariant &selected)
{
    bool selectedOk = false;
    const auto selectedInt = selected.toLongLong(&selectedOk);

    for (const auto &[title, value] : values)
    {
        if (value == selected)
            return title;

        bool valueOk = false;
        const auto valueInt = value.toLongLong(&valueOk);
        if (selectedOk && valueOk && selectedInt == valueInt)
            return title;
    }
    return {};
}

[[nodiscard]] inline QString comboBoxValueTitle(const QVariant &stored, const QVariant &selected)
{
    return comboBoxValueTitle(comboBoxValues(stored), selected);
}

[[nodiscard]] inline QStringList comboBoxValueTitles(const ComboBoxValues &values)
{
    QStringList titles;
    titles.reserve(static_cast<int>(values.size()));
    for (const auto &[title, value] : values)
    {
        Q_UNUSED(value)
        titles.append(title);
    }
    return titles;
}

[[nodiscard]] inline QStringList comboBoxValueTitles(const QVariant &stored)
{
    return comboBoxValueTitles(comboBoxValues(stored));
}

[[nodiscard]] inline QVariant comboBoxValueForTitle(const ComboBoxValues &values, const QString &title)
{
    for (const auto &[itemTitle, itemValue] : values)
    {
        if (itemTitle == title)
            return itemValue;
    }
    return {};
}

struct SettingItem
{
    enum class UiType : uint8_t
    {
        IntLineEdit = 0,
        FloatLineEdit,
        ComboBox,
        Toggle,
        IntSpinBox,
        FloatSpinBox,
        Unknown
    };

    UiType uiType;
    int order;
    bool isGeneral;
    bool isVisible;
    bool IsReadOnly;
    QMetaType::Type type;
    QString name;
    QString title;
    QString titleRu;
    QString description;
    QString descriptionRu;
    int groupId{-1};
    QVariantMap values;

    /*
     * FOR TOGGLE
     * values["default"] = default;
     *
     * FOR COMBOBOX
     * values["default"] = default;
     * values["values"] = QVariant::fromValue(ComboBoxValues{{title1, value1}, {title2, value2}, ...}); // schema oneOf order
     * values["base"] = "uint8_t";
     * values["type"] = "integer";
     *
     * FOR INT LINE EDIT
     * values["default"] = default;
     * values["type"] = "integer";
     * values["base"] = "uint32_t";
     * values["minimum"] = minValue;
     * values["maximum"] = maxValue;
     *
     * FOR FLOAT LINE EDIT
     * values["default"] = default;
     * values["type"] = "number";
     * values["decimals"] = numberOfDecimals;
     * values["minimum"] = minValue;              // Actual minimum value (from exclusiveMinimum or inclusiveMinimum)
     * values["maximum"] = maxValue;              // Actual maximum value (from exclusiveMaximum or inclusiveMaximum)
     * values["isInclusiveMin"] = isInclusiveMin; // true if inclusiveMinimum was used, false if exclusiveMinimum
     * values["isInclusiveMax"] = isInclusiveMax; // true if inclusiveMaximum was used, false if exclusiveMaximum
     * Note: All 4 fields (exclusiveMinimum, inclusiveMinimum, exclusiveMaximum, inclusiveMaximum) can be used
     *       in any combination (e.g., inclusiveMinimum with exclusiveMaximum), but cannot use both
     *       exclusiveMinimum and inclusiveMinimum simultaneously, and cannot use both exclusiveMaximum
     *       and inclusiveMaximum simultaneously.
     *
     * FOR INT SPIN BOX
     * values["default"] = default;
     * values["type"] = "integer";
     * values["base"] = "uint32_t";
     * values["step"] = singleStepValue;
     * values["minimum"] = minValue;
     * values["maximum"] = maxValue;
     *
     * FOR FLOAT SPIN BOX
     * values["default"] = default;
     * values["type"] = "number";
     * values["step"] = singleStepValue;
     * values["minimum"] = minValue;              // Actual minimum value (from exclusiveMinimum or inclusiveMinimum)
     * values["maximum"] = maxValue;              // Actual maximum value (from exclusiveMaximum or inclusiveMaximum)
     * values["isInclusiveMin"] = isInclusiveMin; // true if inclusiveMinimum was used, false if exclusiveMinimum
     * values["isInclusiveMax"] = isInclusiveMax; // true if inclusiveMaximum was used, false if exclusiveMaximum
     * Note: All 4 fields (exclusiveMinimum, inclusiveMinimum, exclusiveMaximum, inclusiveMaximum) can be used
     *       in any combination (e.g., inclusiveMinimum with exclusiveMaximum), but cannot use both
     *       exclusiveMinimum and inclusiveMinimum simultaneously, and cannot use both exclusiveMaximum
     *       and inclusiveMaximum simultaneously.
     */
};
} // namespace client

Q_DECLARE_METATYPE(client::ComboBoxValues)
