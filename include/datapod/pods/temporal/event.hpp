#pragma once

#include <cstdint>
#include <tuple>

#include "../sequential/string.hpp"
#include "stamp.hpp"

namespace datapod {

    /**
     * @brief Event<T> - Generic timestamped event with typed payload (POD when T is POD)
     *
     * Represents a timestamped event with type and payload for logging and event streams.
     * Event types are dictionary-encoded as uint32_t for efficiency.
     *
     * Template struct for event-driven systems.
     * Use aggregate initialization: Event{ts, type, payload}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - timestamp: Time of event in nanoseconds since epoch
     * - event_type: Event type identifier (dictionary encoded)
     * - payload: Event data of type T
     *
     * Use cases:
     * - Event logging and audit trails
     * - System event streams
     * - User action tracking
     * - State change notifications
     * - Error/warning events
     */
    template <typename T> struct Event {
        int64_t timestamp = 0;   // Event time [ns since epoch]
        uint32_t event_type = 0; // Event type ID (dictionary encoded)
        T payload;               // Event data

        auto members() noexcept { return std::tie(timestamp, event_type, payload); }
        auto members() const noexcept { return std::tie(timestamp, event_type, payload); }

        // Construction
        Event() = default;
        Event(int64_t ts, uint32_t type, const T &data) : timestamp(ts), event_type(type), payload(data) {}
        Event(uint32_t type, const T &data) : timestamp(Stamp<int>::now()), event_type(type), payload(data) {}

        // Utilities
        static inline int64_t now() noexcept { return Stamp<int>::now(); }

        inline int64_t age() const noexcept { return now() - timestamp; }

        inline double seconds() const noexcept { return timestamp / 1'000'000'000.0; }

        // Comparison (by timestamp)
        inline bool operator<(const Event &other) const noexcept { return timestamp < other.timestamp; }

        inline bool operator==(const Event &other) const noexcept {
            return timestamp == other.timestamp && event_type == other.event_type && payload == other.payload;
        }

        inline bool operator!=(const Event &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief LogEvent - Structured log event (POD)
     *
     * Standard logging event with message and severity level.
     *
     * Pure aggregate struct for logging.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - message: Log message text
     * - level: Severity level (DEBUG=0, INFO=1, WARN=2, ERROR=3)
     */
    struct LogEvent {
        String message;
        uint8_t level = 0; // DEBUG=0, INFO=1, WARN=2, ERROR=3

        auto members() noexcept { return std::tie(message, level); }
        auto members() const noexcept { return std::tie(message, level); }

        // Construction
        LogEvent() = default;
        LogEvent(const String &msg, uint8_t lvl) : message(msg), level(lvl) {}

        // Log levels
        static constexpr uint8_t DEBUG = 0;
        static constexpr uint8_t INFO = 1;
        static constexpr uint8_t WARN = 2;
        static constexpr uint8_t ERROR = 3;

        // Utilities
        inline bool is_debug() const noexcept { return level == DEBUG; }
        inline bool is_info() const noexcept { return level == INFO; }
        inline bool is_warn() const noexcept { return level == WARN; }
        inline bool is_error() const noexcept { return level == ERROR; }

        // Comparison
        inline bool operator==(const LogEvent &other) const noexcept {
            return message == other.message && level == other.level;
        }

        inline bool operator!=(const LogEvent &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief SystemEvent - System component event (POD)
     *
     * Event representing system component actions.
     *
     * Pure aggregate struct for system events.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - component: Component/subsystem name
     * - action: Action performed
     */
    struct SystemEvent {
        String component;
        String action;

        auto members() noexcept { return std::tie(component, action); }
        auto members() const noexcept { return std::tie(component, action); }

        // Construction
        SystemEvent() = default;
        SystemEvent(const String &comp, const String &act) : component(comp), action(act) {}

        // Comparison
        inline bool operator==(const SystemEvent &other) const noexcept {
            return component == other.component && action == other.action;
        }

        inline bool operator!=(const SystemEvent &other) const noexcept { return !(*this == other); }
    };

    // Common event type aliases
    using LogEventStamped = Event<LogEvent>;
    using SystemEventStamped = Event<SystemEvent>;

    namespace event {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace event

} // namespace datapod
