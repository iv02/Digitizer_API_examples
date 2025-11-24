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
     *
     * FOR INT LINE EDIT
     * values["default"] = default;
     * values["type"] = "integer";
     * values["base"] = "uint32_t";
     * values["minimum"] = minValue;
     * values["maximum"] = "maxvalue";
     *
     * FOR FLOAT LINE EDIT
     * values["default"] = default;
     * values["type"] = "number";
     * values["decimals"] = numberOfDecimals;
     * values["minimum"] = minValue;
     * values["maximum"] = "maxvalue";
     *
     * FOR INT SPIN BOX
     * values["default"] = default;
     * values["type"] = "integer";
     * values["base"] = "uint32_t";
     * values["step"] = singleStepValue;
     * values["minimum"] = minValue;
     * values["maximum"] = "maxvalue";
     *
     * FOR FLOAT SPIN BOX
     * values["default"] = default;
     * values["type"] = "number";
     * values["step"] = singleStepValue;
     * values["minimum"] = minValue;
     * values["maximum"] = "maxvalue";
     */
};
} // namespace client
