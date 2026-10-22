#pragma once

#include "eventpacketbase.h"

#include "packets/waveformnetworkpacket.h"

#include <memory>
#include <vector>

namespace network
{

class WaveformEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    WaveformEventPacket() = default;

    explicit WaveformEventPacket(const std::shared_ptr<WaveformNetworkPacket> &packet);

    void serialize(QDataStream &out) const override;
    void serializePadding(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;
    void deserializePadding(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    quint16 m_decimationFactor{};
    quint16 m_paddingLength{};
    std::vector<qint16> m_waveform{};

  private:
    static constexpr size_t kWaveformScalarFieldsSize = sizeof(quint16) + sizeof(quint16);

    void assignFrom(const WaveformNetworkPacket &packet);
};

} // namespace network
