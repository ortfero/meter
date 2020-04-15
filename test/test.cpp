#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


#include <meter/meter.hpp>



TEST_CASE("sample constructed by default") {

  using std::chrono::steady_clock;

  meter::sample const sample;

  REQUIRE(sample.title.empty());
  REQUIRE(sample.all_hits == 0);
  REQUIRE(sample.period_hits == 0);
  REQUIRE(sample.all_ticks == steady_clock::duration{});
  REQUIRE(sample.period_ticks == steady_clock::duration{});
  REQUIRE(sample.from.time_since_epoch().count() == 0);
  REQUIRE(sample.to.time_since_epoch().count() == 0);
}



TEST_CASE("sample constructed with title") {

  using std::chrono::steady_clock;

  meter::sample const sample{"sample"};

  REQUIRE(sample.title == "sample");
  REQUIRE(sample.all_hits == 0);
  REQUIRE(sample.period_hits == 0);
  REQUIRE(sample.all_ticks == steady_clock::duration{});
  REQUIRE(sample.period_ticks == steady_clock::duration{});
  REQUIRE(sample.from.time_since_epoch().count() == 0);
  REQUIRE(sample.to.time_since_epoch().count() == 0);
}


TEST_CASE("sample constructed from copy") {

  using namespace std::chrono;

  meter::sample sample{"sample"};
  sample.all_hits = 47;
  sample.all_ticks = steady_clock::duration{1947};
  auto const now = system_clock::now();
  sample.to = now;
  sample.from = now - minutes{1};

  meter::sample const copy(sample);

  REQUIRE(copy.title == "sample");
  REQUIRE(copy.all_hits == 47);
  REQUIRE(copy.all_ticks == steady_clock::duration{1947});
  REQUIRE(copy.to == now);
  REQUIRE(copy.from == now - minutes{1});
}




TEST_CASE("default constructed hit counter") {

  using std::chrono::steady_clock;


  meter::hit_counter target;

  REQUIRE(!target);
  REQUIRE(target.empty());

  meter::sample sample;
  target.slice(sample);

  REQUIRE(sample.all_ticks == steady_clock::duration{});
  REQUIRE(sample.all_hits == 0);
}


TEST_CASE("hit the counter") {

  using std::chrono::steady_clock;

  meter::hit_counter hc;
  hc.hit(steady_clock::duration{3});
  hc.hit(steady_clock::duration{9});
  meter::sample sample;

  hc.slice(sample);
  REQUIRE(sample.all_hits == 2);
  REQUIRE(sample.period_hits == 2);
  REQUIRE(sample.all_ticks == steady_clock::duration{12});
  REQUIRE(sample.ticks.average == steady_clock::duration{6});
  REQUIRE(sample.ticks.minimum == steady_clock::duration{3});
  REQUIRE(sample.ticks.maximum == steady_clock::duration{9});

  hc.slice(sample);
  REQUIRE(sample.all_hits == 2);
  REQUIRE(sample.period_hits == 0);
  REQUIRE(sample.all_ticks == steady_clock::duration{});
  REQUIRE(sample.ticks.average == steady_clock::duration{});
  REQUIRE(sample.ticks.minimum == steady_clock::duration{});
  REQUIRE(sample.ticks.maximum == steady_clock::duration{});
}
