# meter
C++17 one-header library for runtime stats

## Snippet
```cpp
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <iostream>

#include <meter/meter.hpp>



int main() {

  using namespace std;

  meter::spinned_hit_counter items_processed;
  atomic_bool done{false};

  cout << "Press ENTER to stop" << endl;

  auto const processor = async(launch::async, [&]{
    using namespace std::chrono;
    while(!done) {
      auto const started = steady_clock::now();
      this_thread::sleep_for(milliseconds{10});
      // time for processed item
      items_processed.hit(steady_clock::now() - started);
    }
  });

  auto const logger = async(launch::async, [&]{
    meter::sample sample{"items processed"};
    while(!done) {
      // get sample for last period
      items_processed.slice(sample);
      cout << sample << endl;
      this_thread::sleep_for(chrono::seconds{1});
    }
  });

  getchar();
  done = true;
  processor.wait();
  logger.wait();

  return 0;
}
```

Possible output:
```
Press ENTER to stop
items processed (hits: 91, average: 10958 us, min: 10025.6 us, max: 11600.7 us)
items processed (hits: 92, average: 10957.4 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 92, average: 10914.3 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 92, average: 10957.9 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 91, average: 10946.5 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 92, average: 10946.9 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 92, average: 10957.1 us, min: 10025.6 us, max: 11618 us)
items processed (hits: 91, average: 10958.1 us, min: 10025.6 us, max: 11618 us)
```
