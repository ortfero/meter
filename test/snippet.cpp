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
