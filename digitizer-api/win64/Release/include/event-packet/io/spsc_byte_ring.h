#pragma once

#include <QtGlobal>

#include <atomic>
#include <cstring>
#include <memory>

namespace network
{

struct ByteSpan
{
    char *data{nullptr};
    qsizetype size{0};

    [[nodiscard]] bool empty() const noexcept { return size <= 0 || data == nullptr; }
};

class SPSCByteRing final
{
  public:
    explicit SPSCByteRing(size_t capacity) : m_storage(std::make_unique<char[]>(capacity)), m_capacity(capacity)
    {
        Q_ASSERT(capacity > 0);
    }

    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

    [[nodiscard]] size_t size() const noexcept
    {
        const size_t write = m_write.load(std::memory_order_acquire);
        const size_t read = m_read.load(std::memory_order_acquire);
        return write - read;
    }

    [[nodiscard]] size_t freeSpace() const noexcept { return m_capacity - size(); }

    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    [[nodiscard]] bool full() const noexcept { return freeSpace() == 0; }

    [[nodiscard]] ByteSpan writableSpan() const noexcept
    {
        if (!m_storage)
            return {};

        const size_t write = m_write.load(std::memory_order_relaxed);
        const size_t read = m_read.load(std::memory_order_acquire);
        const size_t used = write - read;
        const size_t free = m_capacity - used;
        if (free == 0)
            return {};

        const size_t writeIdx = write % m_capacity;
        const size_t toEnd = m_capacity - writeIdx;
        const size_t contiguous = free < toEnd ? free : toEnd;
        return {.data = m_storage.get() + writeIdx, .size = static_cast<qsizetype>(contiguous)};
    }

    void commitWrite(qsizetype bytes) noexcept
    {
        Q_ASSERT(bytes >= 0);
        Q_ASSERT(static_cast<size_t>(bytes) <= freeSpace());
        m_write.fetch_add(static_cast<size_t>(bytes), std::memory_order_release);
    }

    [[nodiscard]] ByteSpan readableSpan() const noexcept
    {
        if (!m_storage)
            return {};

        const size_t read = m_read.load(std::memory_order_relaxed);
        const size_t write = m_write.load(std::memory_order_acquire);
        const size_t used = write - read;
        if (used == 0)
            return {};

        const size_t readIdx = read % m_capacity;
        const size_t toEnd = m_capacity - readIdx;
        const size_t contiguous = used < toEnd ? used : toEnd;
        return {.data = m_storage.get() + readIdx, .size = static_cast<qsizetype>(contiguous)};
    }

    void consume(qsizetype bytes) noexcept
    {
        Q_ASSERT(bytes >= 0);
        Q_ASSERT(static_cast<size_t>(bytes) <= size());
        m_read.fetch_add(static_cast<size_t>(bytes), std::memory_order_release);
    }

    [[nodiscard]] qsizetype write(const char *src, qsizetype bytes) noexcept
    {
        if (!src || bytes <= 0)
            return 0;

        qsizetype written = 0;
        while (written < bytes)
        {
            const ByteSpan span = writableSpan();
            if (span.empty())
                break;

            const qsizetype chunk = qMin(span.size, bytes - written);
            std::memcpy(span.data, src + written, static_cast<size_t>(chunk));
            commitWrite(chunk);
            written += chunk;
        }
        return written;
    }

    void release() noexcept
    {
        m_write.store(0, std::memory_order_release);
        m_read.store(0, std::memory_order_release);
        m_storage.reset();
    }

  private:
    std::unique_ptr<char[]> m_storage;
    const size_t m_capacity;
    std::atomic<size_t> m_write{0};
    std::atomic<size_t> m_read{0};
};

} // namespace network
