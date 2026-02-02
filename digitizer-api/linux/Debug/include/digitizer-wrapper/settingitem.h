#pragma once

#include <QVariantMap>

namespace client
{
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
    QString description;
    QVariantMap values;

    /*
     * FOR TOGGLE
     * values["default"] = default;
     *
     * FOR COMBOBOX
     * values["default"] = default;
     * values["values"] = QVariantMap{{"title1", value1}, {"title2", value2}... };
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
