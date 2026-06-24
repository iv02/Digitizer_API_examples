#pragma once

#include "frameparse/packetwrappers/eventdata.h"
#include "parsing_mode.h"

#include <QObject>
#include <QTcpSocket>
#include <QThread>

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
    explicit MeasurementDataPipeline(quint32 expectedDeviceId = 0, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~MeasurementDataPipeline() override;

    MeasurementDataPipeline(const MeasurementDataPipeline &) = delete;
    MeasurementDataPipeline &operator=(const MeasurementDataPipeline &) = delete;

    void processData(QTcpSocket *socket) const;

  private:
    SocketReader *m_socketReader{nullptr};
    DataParser *m_dataParser{nullptr};
    EventAssembler *m_eventAssembler{nullptr};
    EventSorter *m_eventSorter{nullptr};

    QThread *m_ioThread{nullptr};
    QThread *m_parseThread{nullptr};
    QThread *m_eventThread{nullptr};
    QThread *m_sortThread{nullptr};
};

} // namespace network
