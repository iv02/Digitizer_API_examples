#include "waveformspectrumwidget.h"
#include "dataworker.h"
#include "digitizerinteractor.h"
#include "packetwrappers/eventdata.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QOpenGLWidget>
#include <QThread>

using namespace digi;

WaveformSpectrumWidget::WaveformSpectrumWidget(DigitizerInteractor *interactor, QWidget *parent)
    : QWidget(parent)
    , m_interactor(interactor)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_series(nullptr)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
{
    m_worker = new DataWorker();
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);
    m_workerThread->start();
    
    setupUi();
    setupConnections();
}

WaveformSpectrumWidget::~WaveformSpectrumWidget()
{
    if (m_workerThread)
    {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    if (m_worker)
    {
        m_worker->deleteLater();
    }
}

void WaveformSpectrumWidget::setupUi()
{
    m_chart = new QChart();
    m_chart->setTitle("Waveform / Spectrum");
    m_chart->legend()->hide();

    m_series = new QLineSeries();
    m_chart->addSeries(m_series);

    auto axisX = new QValueAxis();
    axisX->setTitleText("Sample / Bin");
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);

    auto axisY = new QValueAxis();
    axisY->setTitleText("Value");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    
    auto glWidget = new QOpenGLWidget();
    m_chartView->setViewport(glWidget);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_chartView);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void WaveformSpectrumWidget::setupConnections()
{
    connect(m_worker, &DataWorker::waveformDataReady, this, &WaveformSpectrumWidget::updateChart, Qt::QueuedConnection);
}

void WaveformSpectrumWidget::processEventData(const network::EventData &eventData)
{
    QMetaObject::invokeMethod(m_worker, "processWaveformData", Qt::QueuedConnection,
                              Q_ARG(const network::EventData &, eventData));
}

void WaveformSpectrumWidget::updateChart(const ProcessedWaveformData &processedData)
{
    if (processedData.data.isEmpty())
        return;

    m_series->clear();
    m_series->append(processedData.data);

    auto axisX = qobject_cast<QValueAxis *>(m_chart->axes(Qt::Horizontal).first());
    auto axisY = qobject_cast<QValueAxis *>(m_chart->axes(Qt::Vertical).first());

    if (axisX && axisY)
    {
        axisX->setRange(processedData.minX, processedData.maxX);
        axisY->setRange(processedData.minY, processedData.maxY);
    }

    m_chart->update();
}

