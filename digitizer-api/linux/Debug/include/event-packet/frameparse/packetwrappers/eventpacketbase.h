#pragma once

#include "eventpacketheader.h"

#include <QDataStream>
#include <QObject>

namespace network
{

class EventPacket : public QObject
{
    Q_OBJECT
  public:
    [[nodiscard]] EventPacketHeader header() const;
    [[nodiscard]] virtual EventPacketType type() const;

    void setHeader(const EventPacketHeader &newHeader);
    bool operator==(EventPacket &other) const;
    bool operator!=(EventPacket &other) const;

    friend QDataStream &operator<<(QDataStream &out, const EventPacket &packet);
    friend QDataStream &operator>>(QDataStream &in, EventPacket &packet);

  public:
    virtual void serialize(QDataStream &out) const = 0;
    virtual void serializePadding(QDataStream &out) const;
    virtual void deserialize(QDataStream &in) = 0;
    virtual void deserializePadding(QDataStream &in);

  protected:
    [[nodiscard]] virtual bool compare(EventPacket *other) const = 0;

  public:
    quint16 m_checksum{};

  protected:
    EventPacketHeader m_header{};
};

} // namespace network
