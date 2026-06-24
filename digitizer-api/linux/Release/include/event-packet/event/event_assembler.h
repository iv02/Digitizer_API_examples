#pragma once

#include "contract_checks.h"
#include "frameparse/packets/eventpackettype.h"
#include "frameparse/packetwrappers/eventdata.h"
#include "frameparse/packetwrappers/eventpacket.h"
#include "frameparse/parsed_packet.h"
#include "parsing_mode.h"
#include "split_up_assembler.h"

#include <QObject>

#include <array>
#include <memory>
#include <vector>

namespace network
{

class EventAssembler final : public QObject
{
    Q_OBJECT

  signals:
    void eventPairsReady(const std::vector<network::EventData> &pairs);

  public:
    explicit EventAssembler(ParsingMode parsingMode = ParsingMode::Normal, QObject *parent = nullptr);
    ~EventAssembler() override = default;

  public slots:
    void onParsedPackets(const network::ParsedPacketList &packets);

  private:
    void pushInfoPacket(const QSharedPointer<EventPacket> &info);
    void tryEmitPair(const QSharedPointer<EventPacket> &waveform);
    void emitAccumulatedPairs();
    void inspectContract(const ParsedPacket &packet, EventPacketType packetType);
    void flushStuckPackets();
    void handleEmptyOnParsedPackets();

  private:
    std::array<QSharedPointer<EventPacket>, 16> m_pendingInfo;
    std::array<QSharedPointer<EventPacket>, 16> m_pendingWaveform;

    std::vector<EventData> m_accumulatedPairs;

    std::unique_ptr<SplitUpPacketAssembler> m_splitUpAssembler;

    ContractChecks<ParsingMode::Normal> m_normalChecks;
    ContractChecks<ParsingMode::Paranoid> m_paranoidChecks;

    ParsingMode m_parsingMode{ParsingMode::Normal};
    int m_consecutiveEmptyOnParsedPackets{0};
};

} // namespace network
