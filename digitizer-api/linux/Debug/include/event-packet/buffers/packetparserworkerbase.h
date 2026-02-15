#pragma once

#include "packetparser.h"

#include <QObject>
#include <QPair>
#include <QSharedPointer>
#include <QVector>
#include <any>

namespace network
{

class PacketParserWorkerBase : public QObject
{
    Q_OBJECT
  public:
    explicit PacketParserWorkerBase(QObject *parent = nullptr) : QObject(parent)
    {
    }
    ~PacketParserWorkerBase() override = default;

  signals:
    void parsed(const std::any &packet, EventPacketType type, const QByteArray &raw) const;
    void parseFailed(EventError error, EventPacketType type) const;

  public slots:
    virtual void parseBytes(const QByteArray &bytes) = 0;
    virtual void enqueueSlices(const QSharedPointer<QByteArray> &buffer, const QVector<QPair<int, int>> &slices) = 0;
    virtual void enqueuePairJob(const QSharedPointer<QByteArray> &buffer, int infoOffset, int infoLength, int waveOffset, int waveLength)
    {
        Q_UNUSED(buffer)
        Q_UNUSED(infoOffset)
        Q_UNUSED(infoLength)
        Q_UNUSED(waveOffset)
        Q_UNUSED(waveLength)
    }
    virtual void enqueueSingleJob(const QSharedPointer<QByteArray> &buffer, int offset, int length, bool isInfo)
    {
        Q_UNUSED(buffer)
        Q_UNUSED(offset)
        Q_UNUSED(length)
        Q_UNUSED(isInfo)
    }
};

} // namespace network
