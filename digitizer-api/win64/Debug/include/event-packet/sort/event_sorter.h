#pragma once

#include "frameparse/parsed_packet.h"
#include "../parsing_mode.h"

#include <QElapsedTimer>
#include <QObject>
#include <queue>
#include <vector>

namespace network
{

class EventSorter final : public QObject
{
    Q_OBJECT

  signals:
    void parsedPacketsReady(const network::ParsedPacketList &packets);

  public:
    explicit EventSorter(quint64 windowSize = 100'000, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~EventSorter() override = default;

  public slots:
    void onParsedPackets(const network::ParsedPacketList &packets);
    void onMeasurementStarted();
    void onMeasurementStopped();

  private:
    struct GreaterRtc
    {
        bool operator()(const ParsedPacket &a, const ParsedPacket &b) const;
    };

    void flushSafe();
    void flushAll();
    void checkSortedAndMonotonic(const ParsedPacketList &packets) const;

    std::priority_queue<ParsedPacket, std::vector<ParsedPacket>, GreaterRtc> m_queue;
    quint64 m_windowSize;
    quint64 m_maxRtcSeen{0};
    quint64 m_lastEmittedRtc{0};
    mutable QElapsedTimer m_crossBatchWarnThrottle;
    ParsingMode m_parsingMode;
};

} // namespace network
