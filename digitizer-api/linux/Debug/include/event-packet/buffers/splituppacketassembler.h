#pragma once

#include "packets/waveformnetworkpacket.h"

#include <QObject>

namespace network
{

class SplitUpPacketAssembler : public QObject
{
    Q_OBJECT
  public:
    enum class SplitUpFlag
    {
        HasBegin = 0x01,
        HasEnd = 0x02,
        FullPacket = HasBegin | HasEnd,
    };
    Q_DECLARE_FLAGS(SplitUpFlags, SplitUpFlag)

  public:
    SplitUpPacketAssembler(QObject *parent);
    ~SplitUpPacketAssembler() override;

    std::optional<WaveformNetworkPacket> processSplitUpPacket(const WaveformNetworkPacket &packet);

  private:
    std::map<int, std::map<int, WaveformNetworkPacket>> m_data{};
};

} // namespace network

Q_DECLARE_OPERATORS_FOR_FLAGS(network::SplitUpPacketAssembler::SplitUpFlags)