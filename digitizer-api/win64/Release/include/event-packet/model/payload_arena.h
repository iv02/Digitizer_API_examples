#pragma once

#include <QByteArray>
#include <QtGlobal>

#include <mutex>
#include <vector>

namespace network
{

struct PayloadRef
{
    quint32 index{0};
    quint32 size{0};

    [[nodiscard]] bool empty() const noexcept { return size == 0; }
};

class PayloadArena final
{
  public:
    PayloadRef store(const char *data, qsizetype size)
    {
        if (!data || size <= 0)
            return {};

        std::lock_guard lock(m_mutex);
        quint32 index;
        if (!m_free.empty())
        {
            index = m_free.back();
            m_free.pop_back();
            m_slots[index].assign(data, data + size);
        }
        else
        {
            index = static_cast<quint32>(m_slots.size());
            m_slots.emplace_back(data, data + size);
        }
        return PayloadRef{.index = index, .size = static_cast<quint32>(size)};
    }

    [[nodiscard]] const char *data(PayloadRef ref) const
    {
        std::lock_guard lock(m_mutex);
        if (ref.empty() || ref.index >= m_slots.size() || m_slots[ref.index].size() != ref.size)
            return nullptr;
        return m_slots[ref.index].data();
    }

    [[nodiscard]] QByteArray toByteArray(PayloadRef ref) const
    {
        std::lock_guard lock(m_mutex);
        if (ref.empty() || ref.index >= m_slots.size() || m_slots[ref.index].size() != ref.size)
            return {};
        return {m_slots[ref.index].data(), static_cast<int>(ref.size)};
    }

    void release(PayloadRef ref)
    {
        if (ref.empty())
            return;
        std::lock_guard lock(m_mutex);
        if (ref.index >= m_slots.size())
            return;
        m_slots[ref.index].clear();
        m_slots[ref.index].shrink_to_fit();
        m_free.push_back(ref.index);
    }

    void releaseAll(const std::vector<PayloadRef> &refs)
    {
        for (PayloadRef ref : refs)
            release(ref);
    }

  private:
    mutable std::mutex m_mutex;
    std::vector<std::vector<char>> m_slots;
    std::vector<quint32> m_free;
};

} // namespace network
