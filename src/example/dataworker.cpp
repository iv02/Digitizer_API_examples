#include "dataworker.h"
#include "packetwrappers/eventdata.h"
#include "packetwrappers/eventpacket.h"

DataWorker::DataWorker(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ProcessedWaveformData>("ProcessedWaveformData");
}

void DataWorker::processWaveformData(const network::EventData &eventData)
{
    auto processedData = processEventDataInternal(eventData);
    if (!processedData.data.isEmpty())
    {
        emit waveformDataReady(processedData);
    }
}

ProcessedWaveformData DataWorker::processEventDataInternal(const network::EventData &eventData)
{
    ProcessedWaveformData result;
    QVector<QPointF> newData;

    if (eventData.waveformPacket)
    {
        auto waveformPacket = qobject_cast<network::WaveformEventPacket *>(eventData.waveformPacket.get());
        if (waveformPacket)
        {
            const auto &waveform = waveformPacket->m_waveform;
            newData.reserve(static_cast<int>(waveform.size()));
            for (int i = 0; i < static_cast<int>(waveform.size()); ++i)
            {
                newData.append(QPointF(i, waveform[i]));
            }
        }
    }

    if (eventData.infoPacket)
    {
        auto spectrumPacket = qobject_cast<network::SpectrumEventPacket *>(eventData.infoPacket.get());
        if (spectrumPacket)
        {
            const auto &spectrum = spectrumPacket->m_spectrum;
            newData.reserve(static_cast<int>(spectrum.size()));
            for (int i = 0; i < static_cast<int>(spectrum.size()); ++i)
            {
                newData.append(QPointF(i, spectrum[i]));
            }
        }
    }

    if (!newData.isEmpty())
    {
        result.data = newData;
        result.minX = newData.first().x();
        result.maxX = newData.last().x();
        result.minY = newData.first().y();
        result.maxY = newData.first().y();

        for (const auto &point : newData)
        {
            if (point.y() < result.minY)
                result.minY = point.y();
            if (point.y() > result.maxY)
                result.maxY = point.y();
        }
    }

    return result;
}

