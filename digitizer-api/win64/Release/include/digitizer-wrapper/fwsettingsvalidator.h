
#include <QVariant>
#include <vector>

class QModelIndex;
class QVariant;

namespace client
{

struct SettingItem;

class FwSettingsValidator
{
  public:
    using SettingItems = std::vector<client::SettingItem>;
    explicit FwSettingsValidator() = default;
    bool process(const QModelIndex &index, QVariant &value, const SettingItems &settingItems) noexcept;
    static bool clamp(int &value, const QVariant &minV, const QVariant &maxV) noexcept;
    static bool clamp(double &value, const QVariant &minv, const QVariant &maxv) noexcept;
    static bool roundToStep(int &value, const QVariant &step) noexcept;
    static bool roundToStep(double &value, const QVariant &step) noexcept;
    static bool roundToDecimals(double &value, const QVariant &decimals) noexcept;
    static bool resolveComboBoxValue(QVariant &value, const QVariantMap &values) noexcept;
    static bool convertToBool(QVariant &value) noexcept;

    void setProcessValidationEnabled(bool enabled) noexcept;
    bool isProcessValidationEnabled() noexcept;

  private:
    bool m_processValidationEnabled{true};
};

} // namespace client