#pragma once

#include "frameparse/packetwrappers/eventdata.h"
#include "parsing_mode.h"

#include <QObject>
#include <QTcpSocket>
#include <QThread>

#include <memory>
#include <vector>

namespace network
{

class DataParser;
class EventAssembler;
class EventSorter;
class SocketReader;

class MeasurementDataPipeline final : public QObject
{
    Q_OBJECT

  signals:
    void eventPairsReady(const std::vector<EventData> &pairs);

  public:
    explicit MeasurementDataPipeline(int chunkQueueCapacity = 256, quint32 expectedDeviceId = 0, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~MeasurementDataPipeline() override;

    MeasurementDataPipeline(const MeasurementDataPipeline &) = delete;
    MeasurementDataPipeline &operator=(const MeasurementDataPipeline &) = delete;

    void processData(QTcpSocket *socket) const;
    void onMeasurementStarted() const;
    void onMeasurementStopped() const;

  private:
    std::unique_ptr<SocketReader> m_socketReader;
    std::unique_ptr<DataParser> m_dataParser;
    std::unique_ptr<EventAssembler> m_eventAssembler;
    std::unique_ptr<EventSorter> m_eventSorter;

    std::unique_ptr<QThread> m_ioThread;
    std::unique_ptr<QThread> m_parseThread;
    std::unique_ptr<QThread> m_eventThread;
    std::unique_ptr<QThread> m_sortThread;
};

} // namespace network
