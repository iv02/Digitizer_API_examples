#pragma once

#include "packets/consistentchannelspectrum16.h"
#include "packets/consistentchannelspectrum32.h"
#include "packets/waveformnetworkpacket.h"

#include <bitset>

template <typename T>
concept ConsistentChannelSpectrumType = std::same_as<T, network::ConsistentChannelSpectrum16> || std::same_as<T, network::ConsistentChannelSpectrum32>;

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

template <typename T>
std::vector<std::vector<T>> separateArrayByConsistentChannels(std::vector<T> data, std::vector<bool> channelFlags, [[maybe_unused]] const bool isOpenCL)
{
    const size_t activeChannelsCount = std::ranges::count(channelFlags, true);

    if (activeChannelsCount == 0)
    {
        qInfo() << "SeparateByConsistentChannels has no enabled channels";
        return {};
    }

    if (data.size() % activeChannelsCount != 0)
    {
        qInfo() << "SeparateByConsistentChannels data size is not divisible by number of active channels";
        return {};
    }

    std::vector<std::vector<T>> result;
    const size_t samplesPerChannel = data.size() / activeChannelsCount;

    result.resize(channelFlags.size());
    for (size_t i = 0; i < channelFlags.size(); ++i)
    {
        if (channelFlags[i])
            result[i].reserve(samplesPerChannel);
    }

    auto dataIt = data.begin();
    for (size_t i = 0; i < channelFlags.size(); ++i)
    {
        if (channelFlags[i])
        {
            result[i] = std::vector<T>(dataIt, dataIt + samplesPerChannel);
            std::advance(dataIt, samplesPerChannel);
        }
        else
            result[i] = {};
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
            if (flagVector.at(it))
            {
                auto resultWaveform = WaveformNetworkPacket();
                resultWaveform.deviceId = waveform.deviceId;
                resultWaveform.packetType = waveform.packetType;
                resultWaveform.channelId = static_cast<uint16_t>(it);
                resultWaveform.array = separated.at(it);

                result.push_back(resultWaveform);
            }

        return result;
    }

    template <ConsistentChannelSpectrumType T> static std::vector<T> separateConsistentChannels(const T &spectrum)
    {
        constexpr auto bitsLength = static_cast<int>(sizeof(spectrum.channelId) * 8);
        const auto channelBits = std::bitset<bitsLength>(spectrum.channelId);

        std::vector<bool> flagVector;
        flagVector.reserve(bitsLength);
        for (auto it = 0; it < bitsLength; it++)
            flagVector.push_back(channelBits.test(it));

        const auto separated = algorithms::separateArrayByConsistentChannels(spectrum.array, flagVector, false);

        std::vector<T> result;
        result.reserve(separated.size());

        for (auto it = 0; it < bitsLength; it++)
            if (flagVector.at(it))
            {
                auto resultSpectrum = T();
                resultSpectrum.deviceId = spectrum.deviceId;
                resultSpectrum.packetType = spectrum.packetType;
                resultSpectrum.channelId = static_cast<uint16_t>(it);
                resultSpectrum.array = separated.at(it);

                result.push_back(resultSpectrum);
            }

        return result;
    }
};

} // namespace network