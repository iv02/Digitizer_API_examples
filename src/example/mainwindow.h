#pragma once

#include "device.h"

#include <QMainWindow>

class QTextEdit;
class QPushButton;
class QMenu;
class QObject;
class QStandardItemModel;
class QTableView;
class QGroupBox;

namespace fwsr
{
class FwSettingsWrapper;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

  signals:
    void DiscoverDevices();
    void connectDevice(int64_t id);
    void downloadSettings(int64_t id);
    void uploadSettings(int64_t id);
    void deviceSettingList(int64_t id);

  private slots:
    void onDiscoverDevices();
    void onShowLeftPanel(bool chacked = false, QPushButton *sender = nullptr);
    void createFWSettingListMenu();

  private:
    int64_t idCurrentDevice() const noexcept;
    void setupUi();
    QWidget *setupLeftPanelUi();
    QWidget *setupRightPanelUi();
    void setStyle();
    void setConnect();

    void appendText(QTextEdit *edit, const QString &text, int percent);

	fwsr::FwSettingsWrapper *m_fwSettingsWrapper;

    QTextEdit *m_edit;
    QTextEdit *m_editSV;
    QMenu *m_menuFWSettingList;

    QMap<int64_t, QList<QString>> m_devices;
    QStandardItemModel *m_devicesModel;
    QTableView *m_devicesTable;
    QTableView *m_dhsfTable;
    QGroupBox *m_settingsGroup;

    QPushButton *m_pbDiscoverDevices;
    QPushButton *m_pbConnectDevice;
    QPushButton *m_pbDisConnectDevice;
    QPushButton *m_pbDownloadSettings;
    QPushButton *m_pbUploadSettings;
    QPushButton *m_pbDeviceSettingList;
    QPushButton *m_pbFwSettings;
    QPushButton *m_pbSetSettings;

    QPushButton *m_pbSettings;
    QPushButton *m_pbDeviceSettings;
    QPushButton *m_pbPHASettings; 
    QPushButton *m_pbPSDSettings;
    QPushButton *m_pbWAVESettings;
};
