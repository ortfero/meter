#pragma once



#include <chrono>
#include <limits>
#include <mutex>



namespace meter {
    
    
    struct timed_value {
        using count_type = std::uint64_t;
        using time_type = std::int64_t;
        
        count_type count{0};
        time_type time{0};
        time_type time_min{std::numeric_limits<time_type>::max()};
        time_type time_max{std::numeric_limits<time_type>::min()};
        
        template<typename S> S& operator << (S& stream, timed_value const& tv) {
            if(count == 0)
                stream << "{ count: 0, time: 0, min: 0, max: 0 }";
            else
                stream << "{ count: " << tv.count
                       << ", time: " << tv.time
                       << ", min: " << tv.time_min
                       << ", max: " << tv.time_max
                       << " }";
            return stream;
        }
    };
    
    
    template<typename C> struct slice {
        using count_type = timed_value::count_type;
        using time_type = timed_value::time_type;
        using clock_type = C;
        using time_point = typename C::time_point;
        
        time_point time_from;
        time_point time_to;
        timed_value current;
        timed_value total;
        
        
        slice(time_point now) noexcept: time_from{now}, time_to{now} { }
        slice(slice const&) noexcept = default;
        slice& operator = (slice const&) noexcept = default;
        
        slice(time_point time_from, time_point time_to, T current, T total) noexcept
            : time_from{time_from}, time_to{time_to}, current{current}, total{total}
        { }
        
        
        template<typename S> S& operator << (S& stream, slice const& slice) {
            using namespace std::chrono;
            auto const delta = duration_cast<seconds>(slice.time_to - slice.time_from);
            stream << "{ period: " << delta.count()
                   << "secs, current: " << slice.current
                   << ", total: " << slice.total
                   << " }";
            return stream;
        }
        
    }; // value
    
    
    template<typename C, typename L> class counter {
        
        mutable L sync_;
        typename C::time_point last_;
        timed_value current_;
        timed_value total_;

    public:

        using clock_type = C;
        using time_type = timed_value::time_type;

        counter() noexcept: last_{clock_type::now()} { }
        counter(counter const&) = delete;
        counter& operator = (counter const&) = delete;


        counter& operator += (time_type time) noexcept {
            std::unique_lock<L> g{sync_};
            ++current_.count;
            current_.time += time;
            ++total_.count;
            total_.time += time;
            if(time < current_.time_min) {
                current_.time_min = time;
                if(time < total_.time_min)
                    total_.time_min = time;
            }
            if(time > current_.time_max) {
                current_.time_max = time;
                if(time > total_.time_max)
                    total_.time_max = time;
            }
            return *this;
        }


        slice<C> slice() noexcept {
            auto const now = clock_type::now();
            std::unique_lock<L> g{sync_};
            auto const current_time = current_.count == 0 ?
                time_type(0) :
                time_type(double(current_.time) / double(current_.count) + 0.5);
            auto const total_time = total_count_ == 0 ?
                time_type(0) :
                time_type(double(total_.time) / double(total_.count) + 0.5);
            auto const r = struct slice<C>{last_, now,
                timed_value{current_.count, current_time, current_.time_min, current_.time_max},
                timed_value{total_.count, total_time, total_.time_min, total_.time_max}};
            last_ = now;
            current_.count = 0;
            current_.time = 0;
            return r;
        }

    }; // counter
 

} // meter
