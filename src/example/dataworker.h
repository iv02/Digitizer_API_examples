#pragma once

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QMetaType>

namespace network
{
struct EventData;
}

struct ProcessedWaveformData
{
    QVector<QPointF> data;
    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;
};

Q_DECLARE_METATYPE(ProcessedWaveformData)

class DataWorker : public QObject
{
    Q_OBJECT

  public:
    explicit DataWorker(QObject *parent = nullptr);

  public slots:
    void processWaveformData(const network::EventData &eventData);

  signals:
    void waveformDataReady(const ProcessedWaveformData &processedData);

  private:
    ProcessedWaveformData processEventDataInternal(const network::EventData &eventData);
};

