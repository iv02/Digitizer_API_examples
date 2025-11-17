#pragma once

#include <QWidget>

class QPushButton;
class QTableView;

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
    QString currentSettingsType() const;
    bool hasActiveSettings() const;

  private slots:
    void onDeviceSettingsClicked();
    void onPHASettingsClicked();
    void onPSDSettingsClicked();
    void onWAVESettingsClicked();

  private:
    void setupUi();
    void setupConnections();
    void setupTableStyle();
    void showSettingsTable(const QString &fwTypeName);
    void uncheckAllButtons();

    digi::DigitizerInteractor *m_interactor;
    QTableView *m_settingsTable;
    int64_t m_currentDeviceId = -1;

    QPushButton *m_pbDeviceSettings;
    QPushButton *m_pbPHASettings;
    QPushButton *m_pbPSDSettings;
    QPushButton *m_pbWAVESettings;
    QPushButton *m_currentButton = nullptr;
};

