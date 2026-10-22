#pragma once

#include <QtGlobal>

#include <QMetaType>

namespace network
{

struct BufferView
{
    const char *data{nullptr};
    qsizetype size{0};

    [[nodiscard]] bool empty() const noexcept { return data == nullptr || size <= 0; }
};

} // namespace network

Q_DECLARE_METATYPE(network::BufferView)
