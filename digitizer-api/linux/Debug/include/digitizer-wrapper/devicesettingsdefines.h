#pragma once

namespace client
{

namespace device_settings
{
constexpr auto device_table_tag = "Device";
constexpr auto psd_table_tag = "PSD";
constexpr auto pha_table_tag = "PHA";
constexpr auto waveform_table_tag = "WAVEFORM";
constexpr auto none_table_tag = "NONE";

constexpr auto psd_setting_value = 0;
constexpr auto pha_setting_value = 1;
constexpr auto waveform_setting_value = 2;
} // namespace device_settings

namespace dependency
{

constexpr auto setting_name_tag = "settings";
constexpr auto comparison_operator_tag = "compare";
constexpr auto expression_tag = "expression";
constexpr auto dependency_tag = "dependences";

}

}