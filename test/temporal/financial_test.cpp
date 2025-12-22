#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/temporal/financial.hpp>
#include <vector>

using namespace datapod;

// ============================================================================
// TEST: Tick - Construction
// ============================================================================

TEST_CASE("Tick - Default Construction") {
    Tick tick;
    CHECK(true); // Just verify it compiles
}

TEST_CASE("Tick - Construction with all fields") {
    Tick tick{1234567890000000LL, 12345, 100.50, 1000, Tick::TRADE};

    CHECK(tick.timestamp == 1234567890000000LL);
    CHECK(tick.sequence == 12345);
    CHECK(tick.price == doctest::Approx(100.50));
    CHECK(tick.volume == 1000);
    CHECK(tick.side == Tick::TRADE);
}

// ============================================================================
// TEST: Tick - Utilities
// ============================================================================

TEST_CASE("Tick - is_bid/is_ask/is_trade") {
    Tick bid{1000, 1, 100.0, 100, Tick::BID};
    Tick ask{2000, 2, 101.0, 200, Tick::ASK};
    Tick trade{3000, 3, 100.5, 150, Tick::TRADE};

    CHECK(bid.is_bid());
    CHECK_FALSE(bid.is_ask());
    CHECK_FALSE(bid.is_trade());

    CHECK(ask.is_ask());
    CHECK_FALSE(ask.is_bid());
    CHECK_FALSE(ask.is_trade());

    CHECK(trade.is_trade());
    CHECK_FALSE(trade.is_bid());
    CHECK_FALSE(trade.is_ask());
}

TEST_CASE("Tick - side_str") {
    Tick bid{1000, 1, 100.0, 100, Tick::BID};
    Tick ask{2000, 2, 101.0, 200, Tick::ASK};
    Tick trade{3000, 3, 100.5, 150, Tick::TRADE};

    CHECK(std::string(bid.side_str()) == "BID");
    CHECK(std::string(ask.side_str()) == "ASK");
    CHECK(std::string(trade.side_str()) == "TRADE");
}

TEST_CASE("Tick - total_value") {
    Tick tick{1000, 1, 100.50, 1000, Tick::TRADE};
    CHECK(tick.total_value() == doctest::Approx(100500.0));
}

// ============================================================================
// TEST: Tick - Comparison
// ============================================================================

TEST_CASE("Tick - comparison by timestamp then sequence") {
    Tick t1{1000, 1, 100.0, 100, Tick::BID};
    Tick t2{1000, 2, 100.0, 100, Tick::BID}; // Same time, different sequence
    Tick t3{2000, 1, 100.0, 100, Tick::BID}; // Different time

    CHECK(t1 < t2); // Same time, t1.seq < t2.seq
    CHECK(t1 < t3); // t1.time < t3.time
    CHECK(t2 < t3);

    CHECK_FALSE(t2 < t1);
    CHECK_FALSE(t3 < t1);
}

TEST_CASE("Tick - equality") {
    Tick t1{1000, 1, 100.50, 100, Tick::TRADE};
    Tick t2{1000, 1, 100.50, 100, Tick::TRADE};
    Tick t3{1000, 2, 100.50, 100, Tick::TRADE}; // Different sequence

    CHECK(t1 == t2);
    CHECK(t1 != t3);
}

TEST_CASE("Tick - sorting") {
    std::vector<Tick> ticks = {{3000, 1, 100.0, 100, Tick::TRADE},
                               {1000, 2, 100.0, 100, Tick::TRADE},
                               {2000, 1, 100.0, 100, Tick::TRADE},
                               {1000, 1, 100.0, 100, Tick::TRADE}};

    std::sort(ticks.begin(), ticks.end());

    CHECK(ticks[0].timestamp == 1000);
    CHECK(ticks[0].sequence == 1);
    CHECK(ticks[1].timestamp == 1000);
    CHECK(ticks[1].sequence == 2);
    CHECK(ticks[2].timestamp == 2000);
    CHECK(ticks[3].timestamp == 3000);
}

// ============================================================================
// TEST: Tick - Reflection
// ============================================================================

TEST_CASE("Tick - has members() for reflection") {
    Tick tick{1000, 1, 100.50, 500, Tick::BID};

    auto tuple = tick.members();
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<1>(tuple) == 1);
    CHECK(std::get<2>(tuple) == doctest::Approx(100.50));
    CHECK(std::get<3>(tuple) == 500);
    CHECK(std::get<4>(tuple) == Tick::BID);
}

TEST_CASE("Tick - works with to_tuple reflection") {
    Tick tick{1000, 1, 100.50, 500, Tick::ASK};

    auto tuple = to_tuple(tick);
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<4>(tuple) == Tick::ASK);
}

TEST_CASE("Tick - works with for_each_field reflection") {
    Tick tick{1000, 1, 100.50, 500, Tick::TRADE};

    int count = 0;
    for_each_field(tick, [&count](auto &field) { count++; });

    CHECK(count == 5); // timestamp, sequence, price, volume, side
}

// ============================================================================
// TEST: OHLCV - Construction
// ============================================================================

TEST_CASE("OHLCV - Default Construction") {
    OHLCV candle;
    CHECK(true); // Just verify it compiles
}

TEST_CASE("OHLCV - Construction with all fields") {
    OHLCV candle{1234567890000000LL, 100.0, 101.5, 99.8, 100.5, 50000};

    CHECK(candle.timestamp == 1234567890000000LL);
    CHECK(candle.open == doctest::Approx(100.0));
    CHECK(candle.high == doctest::Approx(101.5));
    CHECK(candle.low == doctest::Approx(99.8));
    CHECK(candle.close == doctest::Approx(100.5));
    CHECK(candle.volume == 50000);
}

// ============================================================================
// TEST: OHLCV - Utilities
// ============================================================================

TEST_CASE("OHLCV - range") {
    OHLCV candle{1000, 100.0, 105.0, 98.0, 102.0, 1000};
    CHECK(candle.range() == doctest::Approx(7.0)); // 105 - 98
}

TEST_CASE("OHLCV - body") {
    OHLCV bullish{1000, 100.0, 105.0, 98.0, 103.0, 1000};
    OHLCV bearish{2000, 100.0, 105.0, 98.0, 97.0, 1000};

    CHECK(bullish.body() == doctest::Approx(3.0));  // 103 - 100
    CHECK(bearish.body() == doctest::Approx(-3.0)); // 97 - 100
}

TEST_CASE("OHLCV - is_bullish/is_bearish/is_doji") {
    OHLCV bullish{1000, 100.0, 105.0, 98.0, 103.0, 1000};
    OHLCV bearish{2000, 100.0, 105.0, 98.0, 97.0, 1000};
    OHLCV doji{3000, 100.0, 105.0, 98.0, 100.0, 1000};

    CHECK(bullish.is_bullish());
    CHECK_FALSE(bullish.is_bearish());
    CHECK_FALSE(bullish.is_doji());

    CHECK(bearish.is_bearish());
    CHECK_FALSE(bearish.is_bullish());
    CHECK_FALSE(bearish.is_doji());

    CHECK(doji.is_doji());
    CHECK_FALSE(doji.is_bullish());
    CHECK_FALSE(doji.is_bearish());
}

TEST_CASE("OHLCV - upper_wick and lower_wick") {
    // Bullish candle: open=100, close=103, high=105, low=98
    OHLCV bullish{1000, 100.0, 105.0, 98.0, 103.0, 1000};

    CHECK(bullish.upper_wick() == doctest::Approx(2.0)); // 105 - 103
    CHECK(bullish.lower_wick() == doctest::Approx(2.0)); // 100 - 98

    // Bearish candle: open=100, close=97, high=105, low=96
    OHLCV bearish{2000, 100.0, 105.0, 96.0, 97.0, 1000};

    CHECK(bearish.upper_wick() == doctest::Approx(5.0)); // 105 - 100
    CHECK(bearish.lower_wick() == doctest::Approx(1.0)); // 97 - 96 = 1
}

TEST_CASE("OHLCV - typical_price") {
    OHLCV candle{1000, 100.0, 105.0, 99.0, 102.0, 1000};

    double expected = (105.0 + 99.0 + 102.0) / 3.0;
    CHECK(candle.typical_price() == doctest::Approx(expected));
}

TEST_CASE("OHLCV - vwap") {
    OHLCV candle{1000, 100.0, 105.0, 99.0, 102.0, 1000};

    // Simplified VWAP equals typical price
    CHECK(candle.vwap() == doctest::Approx(candle.typical_price()));
}

// ============================================================================
// TEST: OHLCV - Comparison
// ============================================================================

TEST_CASE("OHLCV - comparison by timestamp") {
    OHLCV c1{1000, 100.0, 101.0, 99.0, 100.5, 1000};
    OHLCV c2{2000, 100.0, 101.0, 99.0, 100.5, 1000};
    OHLCV c3{1000, 110.0, 111.0, 109.0, 110.5, 2000}; // Same time, different prices

    CHECK(c1 < c2);
    CHECK_FALSE(c2 < c1);

    CHECK(c1 == c3); // Same timestamp means equal for comparison
}

TEST_CASE("OHLCV - sorting") {
    std::vector<OHLCV> candles = {{3000, 100.0, 101.0, 99.0, 100.5, 1000},
                                  {1000, 100.0, 101.0, 99.0, 100.5, 1000},
                                  {2000, 100.0, 101.0, 99.0, 100.5, 1000}};

    std::sort(candles.begin(), candles.end());

    CHECK(candles[0].timestamp == 1000);
    CHECK(candles[1].timestamp == 2000);
    CHECK(candles[2].timestamp == 3000);
}

// ============================================================================
// TEST: OHLCV - Reflection
// ============================================================================

TEST_CASE("OHLCV - has members() for reflection") {
    OHLCV candle{1000, 100.0, 105.0, 99.0, 102.0, 50000};

    auto tuple = candle.members();
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<1>(tuple) == doctest::Approx(100.0));
    CHECK(std::get<2>(tuple) == doctest::Approx(105.0));
    CHECK(std::get<3>(tuple) == doctest::Approx(99.0));
    CHECK(std::get<4>(tuple) == doctest::Approx(102.0));
    CHECK(std::get<5>(tuple) == 50000);
}

TEST_CASE("OHLCV - works with to_tuple reflection") {
    OHLCV candle{1000, 100.0, 105.0, 99.0, 102.0, 50000};

    auto tuple = to_tuple(candle);
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<5>(tuple) == 50000);
}

TEST_CASE("OHLCV - works with for_each_field reflection") {
    OHLCV candle{1000, 100.0, 105.0, 99.0, 102.0, 50000};

    int count = 0;
    for_each_field(candle, [&count](auto &field) { count++; });

    CHECK(count == 6); // timestamp, open, high, low, close, volume
}

// ============================================================================
// TEST: Practical Use Cases
// ============================================================================

TEST_CASE("Tick - VWAP calculation from tick stream") {
    std::vector<Tick> ticks = {{1000, 1, 100.50, 1000, Tick::TRADE},
                               {1001, 2, 100.52, 500, Tick::TRADE},
                               {1002, 3, 100.48, 750, Tick::TRADE},
                               {1003, 4, 100.55, 1200, Tick::TRADE},
                               {1004, 5, 100.49, 900, Tick::TRADE}};

    double total_value = 0.0;
    uint64_t total_volume = 0;

    for (auto const &tick : ticks) {
        if (tick.is_trade()) {
            total_value += tick.total_value();
            total_volume += tick.volume;
        }
    }

    double vwap = total_value / total_volume;
    CHECK(vwap == doctest::Approx(100.508).epsilon(0.001));
}

TEST_CASE("OHLCV - Technical analysis patterns") {
    // Hammer pattern (long lower wick, small body)
    OHLCV hammer{1000, 100.0, 101.0, 95.0, 100.5, 10000};

    CHECK(hammer.lower_wick() > hammer.upper_wick());
    CHECK(hammer.lower_wick() > hammer.body() * 2);

    // Shooting star (long upper wick, small body)
    OHLCV star{2000, 100.0, 105.0, 99.5, 100.5, 10000};

    CHECK(star.upper_wick() > star.lower_wick());
}
