#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <type_traits>
#include <variant>

#include <iostream>

namespace fas::detail
{
  class StreamStateBase
  {
  public:
    virtual ~StreamStateBase() = default;
  };

  class StreamContinuation
  {
  public:
    virtual ~StreamContinuation() = default;

    virtual void continueWith(std::shared_ptr<StreamStateBase> readyStream) = 0;
  };

  template <class T> class StreamState : public StreamStateBase
  {
  public:
    bool isFinished() const
    {
      return _isFinished && (_queue.empty() || std::holds_alternative<std::monostate>(_queue.front()));
    }
    bool isReady() const
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return readyImpl();
    }

    void waitReady()
    {
      std::unique_lock<std::mutex> lock(_mutex);
      while (!readyImpl())
        _readyCondition.wait(lock);
    }

    [[nodiscard]] std::optional<std::variant<std::monostate, T>> popValue()
    {
      const std::lock_guard<std::mutex> lock(_mutex);
      assert(!_processingPoppedValue);
      if (_queue.empty())
        return std::nullopt;

      auto value = std::move(_queue.front());
      _queue.pop();
      _processingPoppedValue = true;
      return std::move(value);
    }
    //! @return True if the caller has to execute the continuation for this state
    [[nodiscard]] bool finishedProcessingPoppedValue()
    {
      const std::lock_guard<std::mutex> lock(_mutex);
      _processingPoppedValue = false;
      return !_queue.empty() && _cont;
    }

    //! @return True if the caller has to execute the continuation for this state
    [[nodiscard]] bool pushValue(T value)
    {
      bool executeContinuation = false;
      {
        const std::lock_guard<std::mutex> lock(_mutex);
        assert(!_isFinished);
        executeContinuation = _queue.empty() && !_processingPoppedValue && _cont;
        _queue.push(std::move(value));
      }
      _readyCondition.notify_all();
      return executeContinuation;
    }
    //! @return True if the caller has to execute the continuation for this state
    [[nodiscard]] bool close()
    {
      bool executeContinuation = false;
      {
        const std::lock_guard<std::mutex> lock(_mutex);
        assert(!_isFinished);
        _isFinished = true;
        executeContinuation = _queue.empty() && !_processingPoppedValue && _cont;
        _queue.push(std::monostate{});
      }
      _readyCondition.notify_all();
      return executeContinuation;
    }

    StreamContinuation* getContinuation() { return _cont.get(); }
    void resetContinuation() { _cont = nullptr; }

    StreamContinuation* chain(std::unique_ptr<StreamContinuation> cont)
    {
      const std::lock_guard<std::mutex> lock(_mutex);
      assert(!_cont);
      _cont = std::move(cont);
      assert(!_processingPoppedValue);
      if (!_queue.empty())
        return _cont.get();
      else
        return nullptr;
    }

  private:
    bool readyImpl() const { return _isFinished || !_queue.empty(); }

    std::condition_variable _readyCondition;
    mutable std::mutex _mutex;

    bool _isFinished = false;
    bool _processingPoppedValue = false;

    std::queue<std::variant<std::monostate, T>> _queue; // Last stream element is std::monostate
    std::unique_ptr<StreamContinuation> _cont;
  };

  void executeStream(std::pair<StreamContinuation*, std::shared_ptr<StreamStateBase>> next)
  {
    if (next.first)
      next.first->continueWith(std::move(next.second));
  }

  template <class T, class Executor, class Fn> class StreamContinuationThen : public StreamContinuation
  {
  public:
    using U = std::decay_t<std::invoke_result_t<Fn, T>>;

    StreamContinuationThen(Executor executor, Fn fn, std::shared_ptr<StreamState<U>> sstateNext)
        : _executor(executor), _continuationFn(std::move(fn)), _sstateNext(std::move(sstateNext))
    {
      assert(_sstateNext != nullptr);
    }

    virtual void continueWith(std::shared_ptr<StreamStateBase> readyStream) override
    {
      auto readyStreamTyped = static_cast<StreamState<T>*>(readyStream.get());

      auto val = readyStreamTyped->popValue();
      if (!val.has_value())
      {
        assert(false);
        return; // Nothing to do
      }

      // Stream has ended?
      if (std::holds_alternative<std::monostate>(*val))
      {
        if (_sstateNext->close())
          detail::executeStream({_sstateNext->getContinuation(), _sstateNext});
        readyStreamTyped->resetContinuation();
        return;
      }

      _executor.spawn([self = this, readyStream = std::move(readyStream), readyStreamTyped, sstateNext = _sstateNext,
                       continuationFn = _continuationFn, val = std::move(val)]() {
        // Schedule next continuation (if needed)
        if (sstateNext->pushValue(continuationFn(std::move(std::get<T>(*val)))))
        {
          auto continuation = sstateNext->getContinuation();
          detail::executeStream({continuation, std::move(sstateNext)});
        }

        // Reschedule this continuation (if needed)
        if (readyStreamTyped->finishedProcessingPoppedValue())
          detail::executeStream({self, std::move(readyStream)});
      });
    }

  private:
    Executor _executor;
    Fn _continuationFn;
    std::shared_ptr<StreamState<U>> _sstateNext;
  };

} // namespace fas::detail

namespace fas
{
  template <class T> class Stream
  {
  public:
    Stream(std::shared_ptr<detail::StreamState<T>> sstate) : _sstate(std::move(sstate)) {}
    Stream() : _sstate(nullptr) {}

    [[nodiscard]] bool isValid() const { return _sstate != nullptr; }
    [[nodiscard]] bool isReady() const { return _sstate && _sstate->isReady(); }
    [[nodiscard]] bool isFinished() const { return _sstate && _sstate->isFinished(); }

    [[nodiscard]] std::optional<T> get()
    {
      assert(isValid());
      _sstate->waitReady();
      auto value = _sstate->popValue();
      bool _ = _sstate->finishedProcessingPoppedValue();
      if (!value.has_value() || std::holds_alternative<std::monostate>(*value))
        return std::nullopt;
      else
        return std::move(std::get<T>(*value));
    }

    template <class Executor, class Fn>
    [[nodiscard]] Stream<std::decay_t<std::invoke_result_t<std::decay_t<Fn>, T>>> then(Executor executor,
                                                                                       Fn&& continuation) &&
    {
      assert(isValid());

      using U = std::decay_t<std::invoke_result_t<std::decay_t<Fn>, T>>;
      using ContinuationT = detail::StreamContinuationThen<T, Executor, std::decay_t<Fn>>;

      Stream<U> stream;
      stream._sstate = std::make_shared<detail::StreamState<U>>();
      //      stream._canceled = std::move(_canceled);
      auto next = _sstate->chain(
          std::make_unique<ContinuationT>(std::move(executor), std::forward<Fn>(continuation), stream._sstate));

      detail::executeStream({next, std::move(_sstate)});
      return stream;
    }

  private:
    template <class U> friend class Stream;

    std::shared_ptr<detail::StreamState<T>> _sstate;
  };

  template <class T> class StreamProducer
  {
  public:
    StreamProducer(std::shared_ptr<detail::StreamState<T>> sstate) : _sstate(sstate) {}
    StreamProducer(const StreamProducer&) = delete;
    StreamProducer& operator=(const StreamProducer&) = delete;
    StreamProducer(StreamProducer&&) = default;
    StreamProducer& operator=(StreamProducer&&) = default;

    ~StreamProducer()
    {
      if (_sstate)
        reset();
    }

    void reset()
    {
      if (_sstate->close())
        detail::executeStream({_sstate->getContinuation(), _sstate});
      _sstate = nullptr;
    }

    void send(T value)
    {
      if (_sstate->pushValue(std::move(value)))
        detail::executeStream({_sstate->getContinuation(), _sstate});
    }

  private:
    std::shared_ptr<detail::StreamState<T>> _sstate;
  };

  template <class T> std::pair<Stream<T>, StreamProducer<T>> makeStreamProducer()
  {
    auto sstate = std::make_shared<detail::StreamState<T>>();
    Stream<T> stream(sstate);
    StreamProducer<T> producer(std::move(sstate));
    return {std::move(stream), std::move(producer)};
  }
} // namespace fas
