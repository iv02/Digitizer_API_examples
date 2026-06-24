#pragma once

#include "parsed_packet.h"
#include "parsing_mode.h"

#include <QObject>

#include <optional>

namespace network
{

class SocketReader;

class DataParser final : public QObject
{
    Q_OBJECT
  public:
    explicit DataParser(SocketReader *reader, quint32 expectedDeviceId = 0, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~DataParser() override = default;

  public slots:
    void process();

  signals:
    void parsedPacketsReady(const network::ParsedPacketList &packets);

  private:
    [[nodiscard]] bool takeAllAvailableChunksIntoBuffer();
    [[nodiscard]] ParsedPacketList parseBufferedData(bool &waitingForMoreBytes);
    void removeParsedPrefix(qsizetype bytesParsed);
    void updateTailTracking(bool waitingForMoreBytes);
    void handleIdleNoInputAndNoOutput();

    SocketReader *m_reader{nullptr};
    quint32 m_expectedDeviceId{0};
    ParsingMode m_parsingMode{ParsingMode::Normal};
    QByteArray m_buffer;
    qsizetype m_lastBufferedTailSize{0};
    QByteArray m_lastBufferedTailPrefix;
    int m_stalledTailIterations{0};
    int m_consecutiveIdleProcessCalls{0};
};

} // namespace network
