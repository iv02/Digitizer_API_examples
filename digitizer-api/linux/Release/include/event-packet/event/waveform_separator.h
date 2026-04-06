#pragma once

#include "frameparse/packets/waveformnetworkpacket.h"

#include <bitset>
#include <vector>

namespace network
{

class WaveformSeparator
{
  public:
    static std::vector<WaveformNetworkPacket> separateInterleavedChannels(const WaveformNetworkPacket &waveform);
};

} // namespace network
