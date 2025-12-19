#pragma once

#include <cstring>
#include <string_view>
#include <type_traits>

#include "datagram/containers/array.hpp"
#include "datagram/containers/optional.hpp"
#include "datagram/containers/pair.hpp"
#include "datagram/containers/string.hpp"
#include "datagram/containers/tuple.hpp"
#include "datagram/containers/variant.hpp"
#include "datagram/containers/vector.hpp"
#include "datagram/core/endian.hpp"
#include "datagram/core/equal_to.hpp"
#include "datagram/core/mode.hpp"
#include "datagram/core/verify.hpp"
#include "datagram/reflection/for_each_field.hpp"
#include "datagram/serialization/buf.hpp"
#include "datagram/serialization/serialized_size.hpp"

namespace datagram {

    // =============================================================================
    // Serialization Context
    // =============================================================================

    template <typename Target, Mode M> struct SerializationContext {
        static constexpr Mode MODE = M;

        explicit SerializationContext(Target &t) : target_{t} {}

        // Write raw data to target
        offset_t write(void const *ptr, std::size_t const size, std::size_t const alignment = 0) {
            return target_.write(ptr, size, alignment);
        }

        // Write value at position
        template <typename T> void write(offset_t const pos, T const &val) {
            target_.write(static_cast<std::size_t>(pos), val);
        }

        Target &target_;
    };

    // =============================================================================
    // Serialize Implementation
    // =============================================================================

    // Serialize scalar types (int, float, etc.)
    template <Mode M, typename Ctx, typename T> std::enable_if_t<std::is_scalar_v<T>> serialize(Ctx &ctx, T &value) {
        auto const converted = convert_endian<M>(value);
        ctx.write(&converted, sizeof(T), alignof(T));
    }

    // Helper to detect our container types
    template <typename T> struct is_datagram_container : std::false_type {};
    template <> struct is_datagram_container<String> : std::true_type {};
    template <typename T> struct is_datagram_container<Vector<T>> : std::true_type {};
    template <typename T> struct is_datagram_container<Optional<T>> : std::true_type {};
    template <typename A, typename B> struct is_datagram_container<Pair<A, B>> : std::true_type {};
    template <typename T, std::size_t N> struct is_datagram_container<Array<T, N>> : std::true_type {};
    template <typename... Ts> struct is_datagram_container<Tuple<Ts...>> : std::true_type {};
    template <typename... Ts> struct is_datagram_container<Variant<Ts...>> : std::true_type {};
    template <typename T> constexpr bool is_datagram_container_v = is_datagram_container<decay_t<T>>::value;

    // Serialize aggregate types (structs) using reflection
    template <Mode M, typename Ctx, typename T>
    std::enable_if_t<std::is_class_v<T> && !std::is_scalar_v<T> && !is_datagram_container_v<T>> //
    serialize(Ctx &ctx, T &value) {
        if constexpr (to_tuple_works_v<T>) {
            for_each_field(value, [&](auto &field) { serialize<M>(ctx, field); });
        } else {
            // Fallback: memcpy for non-reflectable types
            ctx.write(&value, sizeof(T), alignof(T));
        }
    }

    // Serialize String
    template <Mode M, typename Ctx> void serialize(Ctx &ctx, String &value) {
        // Write length
        auto const len = value.size();
        serialize<M>(ctx, const_cast<std::size_t &>(len));

        // Write string data
        if (len > 0) {
            ctx.write(value.data(), len, 1);
        }
    }

    // Serialize Vector
    template <Mode M, typename Ctx, typename T> void serialize(Ctx &ctx, Vector<T> &value) {
        // Write size
        auto const sz = value.size();
        serialize<M>(ctx, const_cast<std::size_t &>(sz));

        // Write elements
        for (auto &elem : value) {
            serialize<M>(ctx, elem);
        }
    }

    // Serialize Optional
    template <Mode M, typename Ctx, typename T> void serialize(Ctx &ctx, Optional<T> &value) {
        // Write has_value flag
        auto const has_val = value.has_value();
        serialize<M>(ctx, const_cast<bool &>(has_val));

        // Write value if present
        if (has_val) {
            serialize<M>(ctx, *value);
        }
    }

    // Serialize Pair
    template <Mode M, typename Ctx, typename A, typename B> void serialize(Ctx &ctx, Pair<A, B> &value) {
        serialize<M>(ctx, value.first);
        serialize<M>(ctx, value.second);
    }

    // Serialize Array (fixed-size array)
    template <Mode M, typename Ctx, typename T, std::size_t N> void serialize(Ctx &ctx, Array<T, N> &value) {
        for (auto &elem : value) {
            serialize<M>(ctx, elem);
        }
    }

    // Serialize Tuple
    template <Mode M, typename Ctx, typename... Ts> void serialize(Ctx &ctx, Tuple<Ts...> &value) {
        apply([&](auto &...elems) { (serialize<M>(ctx, elems), ...); }, value);
    }

    // Serialize Variant
    template <Mode M, typename Ctx, typename... Ts> void serialize(Ctx &ctx, Variant<Ts...> &value) {
        // Serialize index
        auto idx = value.index();
        serialize<M>(ctx, idx);

        // Serialize active value
        if (value.valid()) {
            value.apply([&](auto &v) { serialize<M>(ctx, v); });
        }
    }

    // =============================================================================
    // Main serialize entry point
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> ByteBuf serialize(T &el) {
        auto b = Buf{};
        auto ctx = SerializationContext<Buf<ByteBuf>, M>{b};
        serialize<M>(ctx, el);
        return std::move(b.buf_);
    }

    // =============================================================================
    // Deserialization Context
    // =============================================================================

    template <Mode M> struct DeserializationContext {
        static constexpr Mode MODE = M;

        explicit DeserializationContext(std::uint8_t const *data, std::size_t size)
            : data_{data}, size_{size}, pos_{0} {}

        // Read raw data
        void read(void *dest, std::size_t num_bytes) {
            verify(pos_ + num_bytes <= size_, "deserialization: out of bounds read");
            std::memcpy(dest, data_ + pos_, num_bytes);
            pos_ += num_bytes;
        }

        // Skip alignment padding
        void align(std::size_t alignment) {
            if (alignment > 1) {
                auto const remainder = pos_ % alignment;
                if (remainder != 0) {
                    pos_ += alignment - remainder;
                }
            }
        }

        std::uint8_t const *data_;
        std::size_t size_;
        std::size_t pos_;
    };

    // =============================================================================
    // Deserialize Implementation
    // =============================================================================

    // Deserialize scalar types
    template <Mode M, typename Ctx, typename T> std::enable_if_t<std::is_scalar_v<T>> deserialize(Ctx &ctx, T &value) {
        ctx.align(alignof(T));
        T temp;
        ctx.read(&temp, sizeof(T));
        value = convert_endian<M>(temp);
    }

    // Deserialize aggregate types using reflection
    template <Mode M, typename Ctx, typename T>
    std::enable_if_t<std::is_class_v<T> && !std::is_scalar_v<T> && !is_datagram_container_v<T>> //
    deserialize(Ctx &ctx, T &value) {
        if constexpr (to_tuple_works_v<T>) {
            for_each_field(value, [&](auto &field) { deserialize<M>(ctx, field); });
        } else {
            // Fallback: direct read
            ctx.align(alignof(T));
            ctx.read(&value, sizeof(T));
        }
    }

    // Deserialize String
    template <Mode M, typename Ctx> void deserialize(Ctx &ctx, String &value) {
        // Read length
        std::size_t len = 0;
        deserialize<M>(ctx, len);

        // Read string data
        if (len > 0) {
            Vector<char> temp(len);
            ctx.read(temp.data(), len);
            value = String(temp.data(), len);
        } else {
            value = String();
        }
    }

    // Deserialize Vector
    template <Mode M, typename Ctx, typename T> void deserialize(Ctx &ctx, Vector<T> &value) {
        // Read size
        std::size_t sz = 0;
        deserialize<M>(ctx, sz);

        // Read elements
        value.resize(sz);
        for (auto &elem : value) {
            deserialize<M>(ctx, elem);
        }
    }

    // Deserialize Optional
    template <Mode M, typename Ctx, typename T> void deserialize(Ctx &ctx, Optional<T> &value) {
        // Read has_value flag
        bool has_val = false;
        deserialize<M>(ctx, has_val);

        // Read value if present
        if (has_val) {
            T temp;
            deserialize<M>(ctx, temp);
            value = std::move(temp);
        } else {
            value.reset();
        }
    }

    // Deserialize Pair
    template <Mode M, typename Ctx, typename A, typename B> void deserialize(Ctx &ctx, Pair<A, B> &value) {
        deserialize<M>(ctx, value.first);
        deserialize<M>(ctx, value.second);
    }

    // Deserialize Array (fixed-size array)
    template <Mode M, typename Ctx, typename T, std::size_t N> void deserialize(Ctx &ctx, Array<T, N> &value) {
        for (auto &elem : value) {
            deserialize<M>(ctx, elem);
        }
    }

    // Deserialize Tuple
    template <Mode M, typename Ctx, typename... Ts> void deserialize(Ctx &ctx, Tuple<Ts...> &value) {
        apply([&](auto &...elems) { (deserialize<M>(ctx, elems), ...); }, value);
    }

    // Helper to deserialize Variant at runtime index
    template <Mode M, std::size_t I, typename Ctx, typename... Ts>
    void deserialize_variant_at_index(Ctx &ctx, Variant<Ts...> &value, std::size_t idx) {
        if constexpr (I < sizeof...(Ts)) {
            if (I == idx) {
                using T = type_at_index_t<I, Ts...>;
                T temp{};
                deserialize<M>(ctx, temp);
                value = std::move(temp);
            } else {
                deserialize_variant_at_index<M, I + 1>(ctx, value, idx);
            }
        }
    }

    // Deserialize Variant
    template <Mode M, typename Ctx, typename... Ts> void deserialize(Ctx &ctx, Variant<Ts...> &value) {
        // Deserialize index
        std::size_t idx;
        deserialize<M>(ctx, idx);

        // Verify index is valid
        verify(idx < sizeof...(Ts), "variant index out of bounds");

        // Deserialize value at the correct index
        deserialize_variant_at_index<M, 0>(ctx, value, idx);
    }

    // =============================================================================
    // Main deserialize entry point
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> T deserialize(ByteBuf const &buf) {
        T result{};
        auto ctx = DeserializationContext<M>{buf.data(), buf.size()};
        deserialize<M>(ctx, result);
        return result;
    }

    template <Mode M = Mode::NONE, typename T> T deserialize(std::uint8_t const *data, std::size_t size) {
        T result{};
        auto ctx = DeserializationContext<M>{data, size};
        deserialize<M>(ctx, result);
        return result;
    }

    // =============================================================================
    // std::string_view overload
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> T deserialize(std::string_view buf) {
        return deserialize<M, T>(reinterpret_cast<std::uint8_t const *>(buf.data()), buf.size());
    }

    // =============================================================================
    // Unaligned Deserialization (Safe for Network Buffers)
    // =============================================================================

    // Check if a pointer is aligned to a given boundary
    inline bool is_aligned(void const *ptr, std::size_t alignment) noexcept {
        return (reinterpret_cast<std::uintptr_t>(ptr) % alignment) == 0;
    }

    // Safe deserialization from potentially unaligned memory (string_view)
    // This is critical for network buffers (ZeroMQ, UDP, TCP) which may not
    // be aligned to required boundaries (4-byte for int, 8-byte for double).
    // On ARM and strict x86, unaligned reads cause hardware faults (SIGBUS).
    template <Mode M = Mode::NONE, typename T> T copy_from_potentially_unaligned(std::string_view buf) {
        // Use max_align_t for maximum portable alignment
        constexpr std::size_t max_alignment = alignof(std::max_align_t);

        // Check if buffer is already aligned
        if (is_aligned(buf.data(), max_alignment)) {
            // Fast path: deserialize in place (zero-copy)
            return deserialize<M, T>(buf);
        }

        // Slow path: copy to aligned buffer first
        ByteBuf aligned_buf(buf.begin(), buf.end());
        return deserialize<M, T>(aligned_buf);
    }

    // Overload for uint8_t pointer
    template <Mode M = Mode::NONE, typename T>
    T copy_from_potentially_unaligned(std::uint8_t const *data, std::size_t size) {
        return copy_from_potentially_unaligned<M, T>(std::string_view(reinterpret_cast<char const *>(data), size));
    }

    // Overload for char pointer (for convenience)
    template <Mode M = Mode::NONE, typename T> T copy_from_potentially_unaligned(char const *data, std::size_t size) {
        return copy_from_potentially_unaligned<M, T>(std::string_view(data, size));
    }

} // namespace datagram
