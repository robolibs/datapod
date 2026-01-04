#pragma once

#include <cstdint>
#include <tuple>

namespace datapod {

    /**
     * @brief High-frequency trading tick data with sequence numbers
     *
     * Tick represents a single market event (bid, ask, or trade) with
     * nanosecond-precision timestamps and sequence numbers for exact ordering.
     * Designed for high-frequency trading and market data analysis.
     *
     * Features:
     * - Nanosecond timestamp precision
     * - Sequence number for deterministic ordering
     * - Side indicator (bid/ask/trade)
     * - Compact representation
     * - Full serialization via members()
     *
     * Example:
     * @code
     * Tick trade{1234567890000000, 12345, 100.50, 1000, Tick::TRADE};
     * Tick bid{1234567890000001, 12346, 100.49, 500, Tick::BID};
     * @endcode
     */
    struct Tick {
        // ========================================================================
        // SIDE CONSTANTS
        // ========================================================================

        static constexpr uint8_t BID = 0;   ///< Bid (buy) order
        static constexpr uint8_t ASK = 1;   ///< Ask (sell) order
        static constexpr uint8_t TRADE = 2; ///< Executed trade

        // ========================================================================
        // DATA MEMBERS
        // ========================================================================

        int64_t timestamp; ///< Timestamp in nanoseconds since epoch
        int64_t sequence;  ///< Sequence number for ordering
        double price;      ///< Price (e.g., $100.50)
        uint64_t volume;   ///< Volume/quantity (e.g., 1000 shares)
        uint8_t side;      ///< BID, ASK, or TRADE

        // ========================================================================
        // REFLECTION & SERIALIZATION (REQUIRED)
        // ========================================================================

        /**
         * @brief Returns tuple of members for reflection and serialization
         *
         * This enables automatic serialization, deserialization, and reflection
         * through datapod's reflection system without manual registration.
         */
        auto members() { return std::tie(timestamp, sequence, price, volume, side); }

        auto members() const { return std::tie(timestamp, sequence, price, volume, side); }

        // ========================================================================
        // CONSTRUCTION
        // ========================================================================

        /// Default constructor
        Tick() = default;

        /**
         * @brief Construct tick with all fields
         * @param ts Timestamp in nanoseconds
         * @param seq Sequence number
         * @param p Price
         * @param vol Volume
         * @param s Side (BID, ASK, or TRADE)
         */
        constexpr Tick(int64_t ts, int64_t seq, double p, uint64_t vol, uint8_t s)
            : timestamp(ts), sequence(seq), price(p), volume(vol), side(s) {}

        // ========================================================================
        // UTILITIES
        // ========================================================================

        /**
         * @brief Check if this is a bid
         * @return true if side == BID
         */
        bool is_bid() const noexcept { return side == BID; }

        /**
         * @brief Check if this is an ask
         * @return true if side == ASK
         */
        bool is_ask() const noexcept { return side == ASK; }

        /**
         * @brief Check if this is a trade
         * @return true if side == TRADE
         */
        bool is_trade() const noexcept { return side == TRADE; }

        /**
         * @brief Get side as string
         * @return "BID", "ASK", or "TRADE"
         */
        char const *side_str() const noexcept {
            switch (side) {
            case BID:
                return "BID";
            case ASK:
                return "ASK";
            case TRADE:
                return "TRADE";
            default:
                return "UNKNOWN";
            }
        }

        /**
         * @brief Calculate total value (price * volume)
         * @return Total value
         */
        double total_value() const noexcept { return price * static_cast<double>(volume); }

        // ========================================================================
        // COMPARISON OPERATORS (by timestamp, then sequence)
        // ========================================================================

        /**
         * @brief Less than comparison (by timestamp, then sequence)
         */
        constexpr bool operator<(Tick const &other) const noexcept {
            if (timestamp != other.timestamp) {
                return timestamp < other.timestamp;
            }
            return sequence < other.sequence;
        }

        /**
         * @brief Greater than comparison
         */
        constexpr bool operator>(Tick const &other) const noexcept {
            if (timestamp != other.timestamp) {
                return timestamp > other.timestamp;
            }
            return sequence > other.sequence;
        }

        /**
         * @brief Less than or equal comparison
         */
        constexpr bool operator<=(Tick const &other) const noexcept { return !(*this > other); }

        /**
         * @brief Greater than or equal comparison
         */
        constexpr bool operator>=(Tick const &other) const noexcept { return !(*this < other); }

        /**
         * @brief Equality comparison
         */
        constexpr bool operator==(Tick const &other) const noexcept {
            return timestamp == other.timestamp && sequence == other.sequence && price == other.price &&
                   volume == other.volume && side == other.side;
        }

        /**
         * @brief Inequality comparison
         */
        constexpr bool operator!=(Tick const &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief Open-High-Low-Close-Volume candle data
     *
     * OHLCV represents aggregated price data over a time period (e.g., 1 minute).
     * This is the standard format for financial charts and technical analysis.
     *
     * Features:
     * - Standard OHLCV format
     * - Utility methods (range, body, bullish/bearish)
     * - Full serialization via members()
     *
     * Example:
     * @code
     * OHLCV candle{1234567890000000, 100.0, 101.5, 99.8, 100.5, 50000};
     * bool bullish = candle.is_bullish();
     * double range = candle.range();
     * @endcode
     */
    struct OHLCV {
        // ========================================================================
        // DATA MEMBERS
        // ========================================================================

        int64_t timestamp; ///< Candle start time (nanoseconds since epoch)
        double open;       ///< Opening price
        double high;       ///< Highest price in period
        double low;        ///< Lowest price in period
        double close;      ///< Closing price
        uint64_t volume;   ///< Total volume in period

        // ========================================================================
        // REFLECTION & SERIALIZATION (REQUIRED)
        // ========================================================================

        /**
         * @brief Returns tuple of members for reflection and serialization
         */
        auto members() { return std::tie(timestamp, open, high, low, close, volume); }

        auto members() const { return std::tie(timestamp, open, high, low, close, volume); }

        // ========================================================================
        // CONSTRUCTION
        // ========================================================================

        /// Default constructor
        OHLCV() = default;

        /**
         * @brief Construct OHLCV with all fields
         * @param ts Timestamp (candle start time)
         * @param o Open price
         * @param h High price
         * @param l Low price
         * @param c Close price
         * @param v Volume
         */
        constexpr OHLCV(int64_t ts, double o, double h, double l, double c, uint64_t v)
            : timestamp(ts), open(o), high(h), low(l), close(c), volume(v) {}

        // ========================================================================
        // UTILITIES
        // ========================================================================

        /**
         * @brief Calculate price range (high - low)
         * @return Range
         */
        double range() const noexcept { return high - low; }

        /**
         * @brief Calculate candle body (close - open)
         * @return Body size (positive for bullish, negative for bearish)
         */
        double body() const noexcept { return close - open; }

        /**
         * @brief Check if candle is bullish (close > open)
         * @return true if bullish
         */
        bool is_bullish() const noexcept { return close > open; }

        /**
         * @brief Check if candle is bearish (close < open)
         * @return true if bearish
         */
        bool is_bearish() const noexcept { return close < open; }

        /**
         * @brief Check if candle is doji (close == open)
         * @return true if doji
         */
        bool is_doji() const noexcept { return close == open; }

        /**
         * @brief Calculate upper wick (high - max(open, close))
         * @return Upper wick size
         */
        double upper_wick() const noexcept { return high - (open > close ? open : close); }

        /**
         * @brief Calculate lower wick (min(open, close) - low)
         * @return Lower wick size
         */
        double lower_wick() const noexcept { return (open < close ? open : close) - low; }

        /**
         * @brief Calculate typical price ((high + low + close) / 3)
         * @return Typical price
         */
        double typical_price() const noexcept { return (high + low + close) / 3.0; }

        /**
         * @brief Calculate VWAP estimate (assume volume distributed evenly)
         * @return Estimated VWAP
         */
        double vwap() const noexcept {
            return typical_price(); // Simplified - true VWAP needs tick data
        }

        // ========================================================================
        // COMPARISON OPERATORS (by timestamp)
        // ========================================================================

        constexpr bool operator<(OHLCV const &other) const noexcept { return timestamp < other.timestamp; }

        constexpr bool operator>(OHLCV const &other) const noexcept { return timestamp > other.timestamp; }

        constexpr bool operator<=(OHLCV const &other) const noexcept { return timestamp <= other.timestamp; }

        constexpr bool operator>=(OHLCV const &other) const noexcept { return timestamp >= other.timestamp; }

        constexpr bool operator==(OHLCV const &other) const noexcept { return timestamp == other.timestamp; }

        constexpr bool operator!=(OHLCV const &other) const noexcept { return timestamp != other.timestamp; }
    };

    namespace financial {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace financial

} // namespace datapod
