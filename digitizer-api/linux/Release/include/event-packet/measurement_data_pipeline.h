#pragma once

#include "model/event_group.h"
#include "parsing_mode.h"

#include <QObject>
#include <QPointer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

#include <memory>
#include <vector>

namespace network
{

class DataParser;
class PayloadArena;
class SocketReader;
class SortGrouper;

class MeasurementDataPipeline final : public QObject
{
    Q_OBJECT

  signals:
    void eventGroupsReady(const std::vector<EventGroup> &groups);

  public:
    explicit MeasurementDataPipeline(quint32 expectedDeviceId = 0, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~MeasurementDataPipeline() override;

    MeasurementDataPipeline(const MeasurementDataPipeline &) = delete;
    MeasurementDataPipeline &operator=(const MeasurementDataPipeline &) = delete;

    void processData(QTcpSocket *socket);

  private:
    std::unique_ptr<PayloadArena> m_arena;
    SocketReader *m_socketReader{nullptr};
    DataParser *m_dataParser{nullptr};
    SortGrouper *m_sortGrouper{nullptr};

    QThread *m_ioThread{nullptr};
    QThread *m_parseThread{nullptr};
    QThread *m_groupThread{nullptr};

    QTimer *m_pollTimer{nullptr};
    QPointer<QTcpSocket> m_socket;
};

} // namespace network
