#pragma once

#include "eventpacketbase.h"

#include "packets/reducedeventinfopacket.h"

#include <QByteArray>
#include <QByteArrayView>

#include <memory>

namespace network
{

class ReducedInfoEventPacket : public EventPacket
{
    Q_OBJECT
  public:
    ReducedInfoEventPacket() = default;

    explicit ReducedInfoEventPacket(const std::shared_ptr<ReducedEventInfoPacket> &packet);

    void serialize(QDataStream &out) const override;
    void serializePadding(QDataStream &out) const override;
    void deserialize(QDataStream &in) override;
    void deserializePadding(QDataStream &in) override;
    [[nodiscard]] qsizetype partsCount() const;
    [[nodiscard]] QByteArrayView part(qsizetype index) const;

  protected:
    [[nodiscard]] bool compare(EventPacket *other) const override;

  public:
    quint16 m_partSize{};
    quint16 m_paddingLength{};
    QByteArray m_buffer{};

  private:
    static constexpr size_t kReducedInfoScalarFieldsSize = sizeof(quint16) + sizeof(quint16);

    void assignFrom(const ReducedEventInfoPacket &packet);
};

} // namespace network
