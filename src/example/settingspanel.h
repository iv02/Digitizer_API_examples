#pragma once

#include <QWidget>

class QHBoxLayout;
class QPushButton;
class QTableView;
class QStandardItemModel;

namespace digi
{
class DigitizerInteractor;
}

class SettingsPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingsPanel(digi::DigitizerInteractor *interactor, QWidget *parent = nullptr);
    ~SettingsPanel() override = default;

    void showSettings(int64_t deviceId, const QString &fwTypeName);
    void hideSettings();
    void refreshFwTypeButtons();
    QString currentSettingsType() const;
    bool hasActiveSettings() const;

  private slots:
    void onFwTypeButtonClicked();

  private:
    void setupUi();
    void setupConnections();
    void setupTableStyle();
    void updateFwTypeButtons();
    void showSettingsTable(const QString &fwTypeName);
    void updateSettingsModel(const QString &fwTypeName);
    void uncheckAllButtons();

    digi::DigitizerInteractor *m_interactor;
    QTableView *m_settingsTable;
    QStandardItemModel *m_settingsModel;
    QHBoxLayout *m_buttonLayout;
    int64_t m_currentDeviceId = -1;
    QString m_currentFwTypeName;

    QList<QPushButton *> m_fwTypeButtons;
    QPushButton *m_currentButton = nullptr;
};

