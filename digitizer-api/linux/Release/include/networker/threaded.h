#pragma once
#include <QObject>
#include <QThread>

namespace client
{
template <typename T> class Threaded final : public QObject
{
  public:
    Threaded(const Threaded &other) = delete;
    Threaded &operator=(const Threaded &other) = delete;
    Threaded(Threaded &&other) noexcept = default;
    Threaded &operator=(Threaded &&other) noexcept = default;

    template <typename... Args>
    explicit Threaded(Args &&...objectArgs) : QObject(nullptr), m_thread(new QThread(this)), m_object(new T(std::forward<Args>(objectArgs)...))
    {
        m_object->moveToThread(m_thread);
        connect(m_thread, &QThread::finished, m_object, &T::deleteLater);
    }

    explicit Threaded(T *object) : QObject(nullptr), m_thread(new QThread(this)), m_object(object)
    {
        m_object->setParent(nullptr);
        m_object->moveToThread(m_thread);
        connect(m_thread, &QThread::finished, m_object, &T::deleteLater);
    }

    ~Threaded() override
    {
        m_thread->quit();
        m_thread->wait();
    }

    template <typename Func, typename... Args> void onStarted(Func slot, Args &&...args)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            connect(m_thread, &QThread::started, m_object, slot);
        }
        else
        {
            connect(m_thread, &QThread::started, m_object, [this, slot, ... args = std::forward<Args>(args)] { (m_object->*slot)(args...); });
        }
    }

    template <typename Func, typename... Args> void onFinished(Func slot, Args &&...args)
    {
        if constexpr (sizeof...(Args) == 0)
        {
            connect(m_thread, &QThread::finished, m_object, slot);
        }
        else
        {
            connect(m_thread, &QThread::finished, m_object, [this, slot, ... args = std::forward<Args>(args)] { (m_object->*slot)(args...); });
        }
    }

    template <typename Signal, typename Slot> void on(Signal signal, Slot slot)
    {
        connect(m_thread, signal, m_object, slot);
    }

    T *object() const
    {
        return m_object;
    }

    QThread *objectThread() const
    {
        return m_thread;
    }

    void start() const
    {
        m_thread->start();
    }

  private:
    QThread *m_thread{nullptr};
    T *m_object{nullptr};
};

template <typename T, typename... Args> Threaded<T> make_threaded(Args &&...objectArgs)
{
    return Threaded<T>{std::forward<Args>(objectArgs)...};
}
} // namespace client
