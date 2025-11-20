#pragma once

#include <QWidget>
#include "dataworker.h"

class QChart;
class QChartView;
class QLineSeries;
class QThread;

namespace network
{
struct EventData;
}

namespace digi
{
class DigitizerInteractor;
}

class WaveformSpectrumWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit WaveformSpectrumWidget(digi::DigitizerInteractor *interactor, QWidget *parent = nullptr);
    ~WaveformSpectrumWidget() override;

  public slots:
    void processEventData(const network::EventData &eventData);
    void updateChart(const ProcessedWaveformData &processedData);

  private:
    void setupUi();
    void setupConnections();

    digi::DigitizerInteractor *m_interactor;
    QChartView *m_chartView;
    QChart *m_chart;
    QLineSeries *m_series;
    
    DataWorker *m_worker;
    QThread *m_workerThread;
};

