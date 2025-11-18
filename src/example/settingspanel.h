#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;

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
    void onSetButtonClicked();
    void onGetButtonClicked();
    void onGetSettingsListButtonClicked();
    void onFwTypeComboBoxChanged(int index);
    void onRefreshFwTypeButtonClicked();

  private:
    void setupUi();
    void setupConnections();
    void updateFwTypeComboBox();
    void updateChannelComboBox();
    void appendToHistory(const QString &message);
    void clearInputs();

    digi::DigitizerInteractor *m_interactor;
    
    // UI элементы
    QTextEdit *m_historyTextEdit;
    QLineEdit *m_settingNameEdit;
    QLineEdit *m_settingValueEdit;
    QPushButton *m_setButton;
    QPushButton *m_getButton;
    QComboBox *m_fwTypeComboBox;
    QPushButton *m_getSettingsListButton;
    QPushButton *m_refreshFwTypeButton;
    QComboBox *m_channelComboBox;
    
    QVBoxLayout *m_mainLayout;
    
    int64_t m_currentDeviceId = -1;
    QString m_currentFwTypeName;
};

