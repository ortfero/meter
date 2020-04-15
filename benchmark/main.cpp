#include <iostream>
#include <chrono>
#include <meter/meter.hpp>
#include <ubench/ubench.hpp>

//#define ANKERL_NANOBENCH_IMPLEMENT
//#include <nanobench/nanobench.h>




int main() {

  using namespace std;
  using std::chrono::steady_clock;

  meter::hit_counter hc;
  auto const hc_hit = ubench::run([&]{ hc.hit(steady_clock::duration{10}); });
  cout << "hit_counter::hit - " << hc_hit << endl;

  meter::spinned_hit_counter shc;
  auto const shc_hit = ubench::run([&]{ shc.hit(steady_clock::duration{10}); });
  cout << "spinned_hit_counter::hit - " << shc_hit << endl;

  using mutexed_hit_counter = meter::basic_hit_counter<std::mutex>;
  mutexed_hit_counter mhc;
  auto const mhc_hit = ubench::run([&]{ mhc.hit(steady_clock::duration{10}); });
  cout << "mutexed_hit_counter::hit - " << mhc_hit << endl;

  meter::sample sample;
  auto const hc_slice = ubench::run([&]{ hc.slice(sample); });
  cout << "hit_counter::slice - " << hc_slice << endl;

  auto const shc_slice = ubench::run([&]{ shc.slice(sample); });
  cout << "spinned_hit_counter::slice - " << shc_slice << endl;

  auto const mhc_slice = ubench::run([&]{ mhc.slice(sample); });
  cout << "mutexed_hit_counter::slice - " << mhc_slice << endl;

  //using bench = ankerl::nanobench::Bench;

  //bench().run("hit_counter::hit", [&]{ hc.hit(steady_clock::duration{10}); });
  //bench().run("hit_counter::slice", [&]{ hc.slice(sample); });
  //bench().run("spinned_hit_counter::hit", [&]{ shc.hit(steady_clock::duration{10}); });
  //bench().run("spinned_hit_counter::slice", [&]{ shc.slice(sample); });
  //bench().run("mutexed_hit_counter::hit", [&]{ mhc.hit(steady_clock::duration{10}); });
  //bench().run("mutexed_hit_counter::slice", [&]{ mhc.slice(sample); });


  return 0;
}
