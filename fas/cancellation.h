#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

namespace fas::detail
{
  class CancellationCallbackBase
  {
  public:
    virtual ~CancellationCallbackBase() = default;
    virtual void operator()() = 0;
  };

  template <class Executor, class Fn> class CancellationCallback : public CancellationCallbackBase
  {
  public:
    CancellationCallback(Executor executor, Fn fn) : _executor(std::move(executor)), _fn(std::move(fn)) {}

    virtual void operator()() override { _executor.spawn(std::move(_fn)); }

  private:
    Executor _executor;
    Fn _fn;
  };

  class CancellationTokenState
  {
  public:
    bool isCanceled() const { return _canceled.load(); }
    void cancel()
    {
      if (!_canceled.exchange(true))
      {
        std::lock_guard<std::mutex> lock(_callbacksMutex);
        for (auto& callback : _callbacks)
          (*callback)();
        _callbacks.clear();
      }
    }

    template <class Executor, class Fn> void subscribe(Executor executor, Fn&& fn)
    {
      auto callback = std::make_unique<CancellationCallback<std::decay_t<Executor>, std::decay_t<Fn>>>(
          std::forward<Executor>(executor), std::forward<Fn>(fn));

      std::lock_guard<std::mutex> lock(_callbacksMutex);
      if (_canceled.load())
        (*callback)();
      else
      {
        _callbacks.push_back(std::move(callback));
      }
    }

  private:
    std::atomic<bool> _canceled;
    std::mutex _callbacksMutex;
    std::vector<std::unique_ptr<CancellationCallbackBase>> _callbacks;
  };
} // namespace fas::detail

namespace fas
{
  class CancellationToken
  {
  public:
    CancellationToken() : _sstate(nullptr) {}
    CancellationToken(std::shared_ptr<detail::CancellationTokenState> sstate) : _sstate(std::move(sstate)) {}

    void reset() { _sstate = nullptr; }
    bool isValid() { return _sstate != nullptr; }

    bool isCanceled() const { return _sstate->isCanceled(); }
    template <class Executor, class Fn> void subscribe(Executor&& executor, Fn&& fn)
    {
      _sstate->subscribe(std::forward<Executor>(executor), std::forward<Fn>(fn));
    }

  private:
    std::shared_ptr<detail::CancellationTokenState> _sstate;
  };

  class CancellationSource
  {
  public:
    CancellationSource() : _sstate(nullptr) {}
    CancellationSource(std::shared_ptr<detail::CancellationTokenState> sstate) : _sstate(std::move(sstate)) {}

    void reset() { _sstate = nullptr; }
    bool isValid() { return _sstate != nullptr; }

    void cancel() { return _sstate->cancel(); }
    CancellationToken token() { return CancellationToken(_sstate); }

  private:
    std::shared_ptr<detail::CancellationTokenState> _sstate;
  };

  extern CancellationSource makeCancellationSource();
} // namespace fas
