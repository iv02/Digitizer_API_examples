#include "datatablewidget.h"
#include "digitizerinteractor.h"
#include "packetwrappers/eventdata.h"
#include "packetwrappers/eventpacket.h"

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

using namespace digi;

DataTableWidget::DataTableWidget(DigitizerInteractor *interactor, QWidget *parent)
    : QWidget(parent)
    , m_interactor(interactor)
    , m_tableView(nullptr)
    , m_model(nullptr)
{
    setupUi();
    setupConnections();
}

void DataTableWidget::setupUi()
{
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"Поле", "Значение"});
    m_model->setRowCount(0);

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_tableView);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void DataTableWidget::setupConnections()
{
}

void DataTableWidget::processEventData(const network::EventData &eventData)
{
    if (eventData.infoPacket)
    {
        auto packetType = eventData.infoPacket->type();
        if (packetType == network::EventPacketType::PsdEventInfo)
        {
            addPsdEventData(eventData);
        }
        else if (packetType == network::EventPacketType::PhaEventInfo)
        {
            addPhaEventData(eventData);
        }
    }
}

void DataTableWidget::addPsdEventData(const network::EventData &eventData)
{
    auto psdPacket = qobject_cast<network::PsdEventPacket *>(eventData.infoPacket.get());
    if (!psdPacket)
        return;

    m_model->setRowCount(0);

    auto header = psdPacket->header();
    int row = 0;

    auto addField = [this, &row](const QString &fieldName, const QString &value) {
        m_model->insertRow(row);
        m_model->setItem(row, 0, new QStandardItem(fieldName));
        m_model->setItem(row, 1, new QStandardItem(value));
        row++;
    };

    addField("Type", "PSD");
    addField("Device ID", QString::number(header.deviceId));
    addField("Channel", QString::number(header.channelId));
    addField("RTC", QString::number(header.rtc));
    addField("QShort", QString::number(psdPacket->m_qShort));
    addField("QLong", QString::number(psdPacket->m_qLong));
    addField("CFD Y1", QString::number(psdPacket->m_cfdY1));
    addField("CFD Y2", QString::number(psdPacket->m_cfdY2));
    addField("Baseline", QString::number(psdPacket->m_baseline));
    addField("Height", QString::number(psdPacket->m_height));
    addField("Event Counter", QString::number(psdPacket->m_eventCounter));
    addField("Event Counter PSD", QString::number(psdPacket->m_eventCounterPsd));
    addField("PSD Value", QString::number(psdPacket->m_psdValue));

    m_tableView->scrollToTop();
}

void DataTableWidget::addPhaEventData(const network::EventData &eventData)
{
    auto phaPacket = qobject_cast<network::PhaEventPacket *>(eventData.infoPacket.get());
    if (!phaPacket)
        return;

    m_model->setRowCount(0);

    auto header = phaPacket->header();
    int row = 0;

    auto addField = [this, &row](const QString &fieldName, const QString &value) {
        m_model->insertRow(row);
        m_model->setItem(row, 0, new QStandardItem(fieldName));
        m_model->setItem(row, 1, new QStandardItem(value));
        row++;
    };

    addField("Type", "PHA");
    addField("Device ID", QString::number(header.deviceId));
    addField("Channel", QString::number(header.channelId));
    addField("RTC", QString::number(header.rtc));
    addField("Event Counter", QString::number(phaPacket->m_eventCounter));
    addField("Trap Baseline", QString::number(phaPacket->m_trapBaseline));
    addField("Trap Height Mean", QString::number(phaPacket->m_trapHeightMean));
    addField("Trap Height Max", QString::number(phaPacket->m_trapHeightMax));
    addField("RC Cr2 Y1", QString::number(phaPacket->m_rcCr2Y1));
    addField("RC Cr2 Y2", QString::number(phaPacket->m_rcCr2Y2));

    m_tableView->scrollToTop();
}

