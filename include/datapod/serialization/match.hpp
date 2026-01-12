#pragma once

/// @file match.hpp
/// @brief Type-safe message dispatch based on serialized type hash
///
/// Provides a fluent API for matching serialized messages against known types
/// and dispatching to appropriate handlers.
///
/// Example:
/// ```cpp
/// using namespace datapod;
///
/// auto result = match(buf)
///     .on<RobotPose>([](auto& p) { handle_pose(p); })
///     .on<RobotHeading>([](auto& h) { handle_heading(h); })
///     .on<SensorData>([](auto& s) { handle_sensors(s); });
///
/// if (!result) {
///     log_error("Unknown message type");
/// }
/// ```

#include "datapod/pods/adapters/result.hpp"
#include "datapod/pods/adapters/tuple.hpp"
#include "datapod/serialization/serialize.hpp"
#include "datapod/type_hash/type_hash.hpp"

namespace datapod {

    namespace detail {

        /// Entry storing a type's hash and its handler
        template <typename T, typename Handler> struct MatchEntry {
            Handler handler;

            bool try_handle(ByteBuf const &buf, hash_t target_hash) const {
                if (target_hash == type_hash<T>()) {
                    auto val = deserialize<Mode::WITH_VERSION, T>(buf);
                    handler(val);
                    return true;
                }
                return false;
            }
        };

        /// Matcher class - accumulates type handlers and executes matching
        template <typename... Entries> struct Matcher {
            ByteBuf const &buf_;
            Tuple<Entries...> entries_;

            Matcher(ByteBuf const &buf, Tuple<Entries...> entries) : buf_(buf), entries_(std::move(entries)) {}

            /// Add a handler for type T
            template <typename T, typename Handler> auto on(Handler &&h) && {
                auto entry = MatchEntry<T, decay_t<Handler>>{std::forward<Handler>(h)};
                auto entry_tuple = Tuple<MatchEntry<T, decay_t<Handler>>>{std::move(entry)};
                auto new_entries = tuple_cat(entries_, entry_tuple);
                using NewMatcher = Matcher<Entries..., MatchEntry<T, decay_t<Handler>>>;
                return NewMatcher{buf_, std::move(new_entries)};
            }

            /// Execute the match - converts to Result<void, Error>
            operator Result<void, Error>() && {
                hash_t hash = peek_type_hash(buf_);
                bool matched = try_match_all(hash, std::make_index_sequence<sizeof...(Entries)>{});
                if (matched) {
                    return Result<void, Error>::ok();
                }
                return Result<void, Error>::err(Error::not_found("no handler matched the message type hash"));
            }

          private:
            template <usize... Is> bool try_match_all(hash_t hash, std::index_sequence<Is...>) const {
                return (try_one<Is>(hash) || ...);
            }

            template <usize I> bool try_one(hash_t hash) const { return get<I>(entries_).try_handle(buf_, hash); }
        };

        /// Initial matcher (no entries yet) - separate type to avoid empty Tuple issues
        struct InitialMatcher {
            ByteBuf const &buf_;

            explicit InitialMatcher(ByteBuf const &buf) : buf_(buf) {}

            template <typename T, typename Handler> auto on(Handler &&h) && {
                auto entry = MatchEntry<T, decay_t<Handler>>{std::forward<Handler>(h)};
                auto entries = Tuple<MatchEntry<T, decay_t<Handler>>>{std::move(entry)};
                using NewMatcher = Matcher<MatchEntry<T, decay_t<Handler>>>;
                return NewMatcher{buf_, std::move(entries)};
            }

            /// Empty matcher always fails
            operator Result<void, Error>() && {
                return Result<void, Error>::err(Error::invalid_argument("match() called with no handlers"));
            }
        };

    } // namespace detail

    /// Entry point for type-matching on serialized buffers
    ///
    /// Usage:
    /// ```cpp
    /// auto result = match(buf)
    ///     .on<TypeA>([](TypeA& a) { /* handle */ })
    ///     .on<TypeB>([](TypeB& b) { /* handle */ });
    ///
    /// if (!result) {
    ///     // No handler matched
    /// }
    /// ```
    inline auto match(ByteBuf const &buf) { return detail::InitialMatcher{buf}; }

} // namespace datapod
