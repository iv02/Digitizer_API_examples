#pragma once

#include "packets/detectron2dnetworkpacket.h"
#include "packets/detectronstatisticnetworkpacket.h"
#include "packets/devicespectrum16.h"
#include "packets/devicespectrum32.h"
#include "packets/phanetworkpacket.h"
#include "packets/psdnetworkpacket.h"
#include "packets/psdnetworkpacketv2.h"
#include "packets/reducedeventinfopacket.h"
#include "packets/waveformnetworkpacket.h"

#include <memory>
#include <variant>
#include <vector>

#include <QMetaType>

namespace network
{

using ParsedPacket = std::variant<
    std::shared_ptr<PsdNetworkPacket>,
    std::shared_ptr<PsdNetworkPacketV2>,
    std::shared_ptr<PhaNetworkPacket>,
    std::shared_ptr<ReducedEventInfoPacket>,
    std::shared_ptr<WaveformNetworkPacket>,
    std::shared_ptr<DeviceSpectrum16>,
    std::shared_ptr<DeviceSpectrum32>,
    std::shared_ptr<DetectronStatisticNetworkPacket>,
    std::shared_ptr<Detectron2dNetworkPacket>>;

using ParsedPacketList = std::vector<ParsedPacket>;

} // namespace network

Q_DECLARE_METATYPE(network::ParsedPacketList)
