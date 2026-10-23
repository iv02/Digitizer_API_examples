#pragma once

#include <QRegularExpression>
#include <QStringList>

namespace client
{

namespace device_settings
{
constexpr auto device_table_tag = "Device";
constexpr auto psd_table_tag = "PSD";
constexpr auto pha_table_tag = "PHA";
constexpr auto waveform_table_tag = "WAVEFORM";
constexpr auto spectrum_table_tag = "Spectrum";
constexpr auto none_table_tag = "NONE";

inline QString canonicalTabName(const QString &tabName)
{
    static const QStringList knownTabNames{device_table_tag, psd_table_tag, pha_table_tag, waveform_table_tag, spectrum_table_tag};

    for (const auto &knownTabName : knownTabNames)
    {
        const QRegularExpression tabExpression(
            QStringLiteral("^%1$").arg(QRegularExpression::escape(knownTabName)),
            QRegularExpression::CaseInsensitiveOption);
        if (tabExpression.match(tabName).hasMatch())
            return knownTabName;
    }

    return tabName;
}

} // namespace device_settings

namespace dependency
{

constexpr auto setting_name_tag = "settings";
constexpr auto comparison_operator_tag = "compare";
constexpr auto expression_tag = "expression";
constexpr auto dependency_tag = "dependences";

}

}