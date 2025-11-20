#pragma once

#include <QWidget>

class QTableView;
class QStandardItemModel;

namespace digi
{
class DigitizerInteractor;
}

namespace network
{
struct EventData;
}

class DataTableWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit DataTableWidget(digi::DigitizerInteractor *interactor, QWidget *parent = nullptr);
    ~DataTableWidget() override = default;

  public slots:
    void processEventData(const network::EventData &eventData);

  private:
    void setupUi();
    void setupConnections();
    void addPsdEventData(const network::EventData &eventData);
    void addPhaEventData(const network::EventData &eventData);

    digi::DigitizerInteractor *m_interactor;
    QTableView *m_tableView;
    QStandardItemModel *m_model;
};

