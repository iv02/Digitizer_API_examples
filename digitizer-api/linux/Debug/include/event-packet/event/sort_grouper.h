#pragma once

#include "contract_checks.h"
#include "model/event_group.h"
#include "model/parsed_event.h"
#include "model/payload_arena.h"
#include "parsing_mode.h"
#include "split_up_assembler.h"

#include "frameparse/packets/waveformnetworkpacket.h"

#include <QObject>

#include <array>
#include <memory>
#include <queue>
#include <vector>

namespace network
{

class SortGrouper final : public QObject
{
    Q_OBJECT

  signals:
    void eventGroupsReady(const std::vector<EventGroup> &groups);

  public:
    explicit SortGrouper(PayloadArena *arena, quint64 windowSize = 100'000, ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~SortGrouper() override = default;

  public slots:
    void onParsedEvents(const ParsedEventList &events);
    void onIdleTick();

  private:
    struct GreaterRtc
    {
        bool operator()(const ParsedEvent &a, const ParsedEvent &b) const;
    };

    void flushSorted();
    void ingest(const ParsedEvent &event);
    void ingestInfo(const ParsedEvent &event);
    void ingestWaveform(const ParsedEvent &event);
    void ingestSplitUp(const ParsedEvent &event);
    void ingestInterleaved(const ParsedEvent &event);
    void flushPendingWaveform(size_t ch);
    void flushStalePendingInfos(size_t ch, quint64 newerInfoRtc);
    void flushStuck();
    void emitGroups();
    void resetContracts();

    [[nodiscard]] WaveformNetworkPacket deserializeWaveform(const ParsedEvent &event) const;
    [[nodiscard]] ParsedEvent storeWaveformEvent(const WaveformNetworkPacket &waveform, EventPacketType packetType);
    [[nodiscard]] static quint64 waveformEndRtc(const WaveformNetworkPacket &w);
    [[nodiscard]] static bool rtcInWaveformRange(quint64 infoRtc, quint64 waveRtc, quint64 endRtc);

    PayloadArena *m_arena{nullptr};
    quint64 m_windowSize{};
    quint64 m_maxRtcSeen{0};
    ParsingMode m_parsingMode{ParsingMode::Normal};
    int m_idleCycles{0};

    std::priority_queue<ParsedEvent, std::vector<ParsedEvent>, GreaterRtc> m_queue;
    std::unique_ptr<SplitUpPacketAssembler> m_splitUpAssembler;

    ContractChecks<ParsingMode::Normal> m_normalChecks;
    ContractChecks<ParsingMode::Paranoid> m_paranoidChecks;

    static constexpr size_t kChannelCount = 16;
    std::array<std::vector<ParsedEvent>, kChannelCount> m_pendingInfos;
    std::array<ParsedEvent, kChannelCount> m_pendingWaveform;
    std::array<bool, kChannelCount> m_hasPendingWaveform{};
    std::array<quint64, kChannelCount> m_pendingWaveformEndRtc{};
    std::array<std::vector<ParsedEvent>, kChannelCount> m_pendingWaveformInfos;

    std::vector<EventGroup> m_out;
    quint64 m_nextSeq{0};
};

} // namespace network
