#pragma once



#include <cstdint>
#include <limits>
#include <chrono>
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <iosfwd>



namespace meter {



template<typename F> std::chrono::steady_clock::duration run(F&& f) {

  using std::chrono::steady_clock;

  auto const started = steady_clock::now();
  f();
  return steady_clock::now() - started;

}



template<typename T> struct distributed_value {

  T average{0};
  T minimum{0};
  T maximum{0};

  distributed_value() = default;
  distributed_value(distributed_value const&) = default;
  distributed_value& operator = (distributed_value const&) = default;

  distributed_value(T const& average):
    average{average}
  { }

  distributed_value(T const& average, T const& min, T const& max):
    average{average}, minimum{min}, maximum{max}
  { }

}; // distributed_value



struct sample {

  using hits_type = size_t;
  using system_clock = std::chrono::system_clock;
  using steady_clock = std::chrono::steady_clock;
  using micros = std::chrono::duration<double, std::micro>;
  using ticks_type = steady_clock::duration::rep;

  static constexpr auto min_ticks =
      steady_clock::duration{std::numeric_limits<ticks_type>::min()};
  static constexpr auto max_ticks =
      steady_clock::duration{std::numeric_limits<ticks_type>::max()};

  std::string title;
  system_clock::time_point from;
  system_clock::time_point to;
  hits_type all_hits{0};
  hits_type period_hits{0};
  steady_clock::duration all_ticks{0};
  steady_clock::duration period_ticks{0};
  distributed_value<steady_clock::duration> ticks{steady_clock::duration{}, max_ticks, min_ticks};
  distributed_value<micros> microseconds;

  sample() = default;
  sample(sample const&) = default;
  sample& operator = (sample const&) = default;
  sample(sample&&) noexcept = default;
  sample& operator = (sample&&) noexcept = default;
  sample(std::string title) noexcept: title{std::move(title)} {}


  explicit operator micros () noexcept {
    return microseconds.average;
  }


  template<typename charT, typename traits> friend
    std::basic_ostream<charT, traits>&
      operator << (std::basic_ostream<charT, traits>& os, sample const& s) noexcept {
        os << s.title;
        if(s.period_hits == 0)
          return os << " (no hits)";
        os << " (hits: " << s.period_hits << ", average: " << s.microseconds.average.count()
           << " us, min: " << s.microseconds.minimum.count() << " us, max: " << s.microseconds.maximum.count() << " us)";
        return os;
      }

}; // sample



namespace detail {



struct config {
  static constexpr size_t cacheline_size = 64;
}; // config



struct nolock {

  nolock() noexcept = default;
  nolock(nolock const&) noexcept = delete;
  nolock& operator = (nolock const&) noexcept = delete;

  bool try_lock() noexcept { return true; }
  void unlock() noexcept { }
  void lock() noexcept { }

}; // nolock



struct spinlock {

  spinlock() noexcept = default;
  spinlock(spinlock const&) noexcept = delete;
  spinlock& operator = (spinlock const&) noexcept = delete;


  bool try_lock() noexcept {
    if(flag_.load(std::memory_order_relaxed))
      return false;
    return !flag_.exchange(true, std::memory_order_acquire);
  }


  void unlock() noexcept {
    flag_.store(false, std::memory_order_release);
  }

  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }


private:

  alignas(config::cacheline_size) std::atomic_bool flag_{false};

}; // spinlock


} // detail



template<typename S = detail::nolock>
struct basic_hit_counter {

  using hits_type = sample::hits_type;
  using steady_clock = std::chrono::steady_clock;
  using sync_type = S;

  basic_hit_counter() noexcept = default;
  basic_hit_counter(basic_hit_counter const&) noexcept = default;
  basic_hit_counter& operator = (basic_hit_counter const&) noexcept = default;



  explicit operator bool () const noexcept {
    std::unique_lock g(sync_);
    return hits_ != 0;
  }



  bool empty() const noexcept {
    std::unique_lock g(sync_);
    return hits_ == 0;
  }



  void hit(steady_clock::duration ticks) noexcept {
    std::unique_lock g(sync_);
    ++hits_;
    sum_ticks_ += ticks;
    update_min_max(ticks);
  }



  void hit(hits_type hits, steady_clock::duration ticks) {
    std::unique_lock g(sync_);
    hits_ += hits;
    sum_ticks_ += ticks;
    auto const average_ticks = steady_clock::duration::rep(
                                 double(ticks.count()) / hits + 0.5);
    update_min_max(steady_clock::duration{average_ticks});
  }



  sample& slice(sample& s) const noexcept {

    using namespace std::chrono;

    std::unique_lock g(sync_);

    sample current;

    s.from = s.to;
    s.to = std::chrono::system_clock::now();

    s.period_hits = hits_ - s.all_hits;
    s.all_hits = hits_;

    if(min_ticks_ < s.ticks.minimum) {
      s.ticks.minimum = min_ticks_;
      s.microseconds.minimum = duration_cast<nanoseconds>(min_ticks_);
    }

    if(max_ticks_ > s.ticks.maximum) {
      s.ticks.maximum = max_ticks_;
      s.microseconds.maximum = duration_cast<nanoseconds>(max_ticks_);
    }

    s.period_ticks = sum_ticks_ - s.all_ticks;
    s.all_ticks = sum_ticks_;
    auto const average = sample::ticks_type(s.period_ticks.count() / double(s.period_hits) + 0.5);
    s.ticks.average = steady_clock::duration{average};
    s.microseconds.average = duration_cast<nanoseconds>(s.ticks.average);

    return s;
  }



private:

  hits_type hits_{0};
  steady_clock::duration sum_ticks_{0};
  steady_clock::duration min_ticks_{sample::max_ticks};
  steady_clock::duration max_ticks_{sample::min_ticks};
  mutable sync_type sync_;



  void update_min_max(steady_clock::duration t) {

    if(t < min_ticks_)
      min_ticks_ = t;
    if(t > max_ticks_)
      max_ticks_ = t;

  }

}; // basic_hit_counter



using hit_counter = basic_hit_counter<detail::nolock>;

using spinned_hit_counter = basic_hit_counter<detail::spinlock>;



} // meter
