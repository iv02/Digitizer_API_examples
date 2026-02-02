#pragma once

#include "packets/waveformnetworkpacket.h"

#include <bitset>

namespace algorithms
{

template <typename T>
std::vector<std::vector<T>> separateWaveformByInterleavedChannels(std::vector<T> waveformData, std::vector<bool> channelFlags,
                                                                  [[maybe_unused]] const bool isOpenCL)
{
    const size_t activeChannelsCount = std::ranges::count(channelFlags, true);

    std::vector<std::vector<T>> result;
    const size_t samplesPerChannel = waveformData.size() / activeChannelsCount;

    result.resize(channelFlags.size());
    for (size_t i = 0; i < channelFlags.size(); ++i)
    {
        if (channelFlags[i])
            result[i].reserve(samplesPerChannel);
    }

    size_t currentChannel = 0;
    auto pickNextEnabledChannel = [&channelFlags, &currentChannel] { currentChannel = (currentChannel + 1) % channelFlags.size(); };
    auto isChannelEnabled = [&channelFlags, &currentChannel] { return channelFlags[currentChannel]; };

    for (double sample : waveformData)
    {
        while (!isChannelEnabled())
            pickNextEnabledChannel();

        result[currentChannel].push_back(static_cast<T>(sample));
        pickNextEnabledChannel();
    }

    return result;
}

} // namespace algorithms

namespace network
{

class WaveWaveformSeparator
{
  public:
    static std::vector<WaveformNetworkPacket> separateInterleavedChannels(const WaveformNetworkPacket &waveform)
    {
        constexpr auto bitsLength = static_cast<int>(sizeof(waveform.channelId) * 8);
        const auto channelBits = std::bitset<bitsLength>(waveform.channelId);

        std::vector<bool> flagVector;
        flagVector.reserve(bitsLength);
        for (auto it = 0; it < bitsLength; it++)
            flagVector.push_back(channelBits.test(it));

        const auto separated = algorithms::separateWaveformByInterleavedChannels(waveform.array, flagVector, false);

        std::vector<WaveformNetworkPacket> result;
        result.reserve(separated.size());

        for (auto it = 0; it < bitsLength; it++)
            if (flagVector[it])
            {
                auto resultWaveform = WaveformNetworkPacket();
                resultWaveform.deviceId = waveform.deviceId;
                resultWaveform.packetType = waveform.packetType;
                resultWaveform.channelId = static_cast<uint16_t>(it);
                resultWaveform.array = separated[it];

                result.push_back(resultWaveform);
            }

        return result;
    }
};

} // namespace network