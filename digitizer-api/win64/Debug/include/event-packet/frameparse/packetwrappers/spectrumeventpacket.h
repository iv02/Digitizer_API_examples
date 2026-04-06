#pragma once

#include "eventpacketbase.h"

#include "packets/devicespectrum16.h"
#include "packets/devicespectrum32.h"
#include "packets/spectrumtype.h"

#include <memory>
#include <vector>

namespace network
{

class SpectrumEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    SpectrumEventPacket() = default;

    explicit SpectrumEventPacket(const std::shared_ptr<DeviceSpectrum16> &packet16);
    explicit SpectrumEventPacket(const std::shared_ptr<DeviceSpectrum32> &packet32);

    void serialize(QDataStream &out) const override;
    void serializePadding(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;
    void deserializePadding(QDataStream &in) override;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    SpectrumType m_spectrumType{};
    quint16 m_paddingLength{};
    std::vector<qint32> m_spectrum{};

  private:
    void assignScalars(const DeviceSpectrum16 &packet);
    void assignScalars(const DeviceSpectrum32 &packet);
};

} // namespace network
