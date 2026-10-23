#pragma once

#include "io/buffer_view.h"
#include "model/parsed_event.h"
#include "model/payload_arena.h"
#include "parsing_mode.h"

#include <QByteArray>
#include <QObject>

namespace network
{

class SocketReader;

class DataParser final : public QObject
{
    Q_OBJECT

  signals:
    void parsedEventsReady(const ParsedEventList &events);

  public:
    explicit DataParser(SocketReader *reader, PayloadArena *arena, quint32 expectedDeviceId = 0, ParsingMode parsingMode = ParsingMode::Normal,
                        QObject *parent = nullptr);
    ~DataParser() override = default;

  public slots:
    void process(network::BufferView view);

  private:
    [[nodiscard]] qsizetype parseSpan(const char *data, qsizetype size, ParsedEventList &out, bool &waitingForMoreBytes);
    void updateTailTracking(bool waitingForMoreBytes);
    void handleIdleNoInputAndNoOutput();

    SocketReader *m_reader{nullptr};
    PayloadArena *m_arena{nullptr};
    quint32 m_expectedDeviceId{0};
    ParsingMode m_parsingMode{ParsingMode::Normal};
    QByteArray m_carry;

    qsizetype m_lastBufferedTailSize{0};
    QByteArray m_lastBufferedTailPrefix;
    int m_stalledTailIterations{0};
    int m_consecutiveIdleProcessCalls{0};
    quint64 m_nextSeq{0};
};

} // namespace network
