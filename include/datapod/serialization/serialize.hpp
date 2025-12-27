#pragma once

#include <cstring>
#include <string_view>
#include <type_traits>

#include "datapod/adapters/optional.hpp"
#include "datapod/adapters/pair.hpp"
#include "datapod/adapters/tuple.hpp"
#include "datapod/adapters/variant.hpp"
#include "datapod/associative/map.hpp"
#include "datapod/associative/set.hpp"
#include "datapod/core/endian.hpp"
#include "datapod/core/equal_to.hpp"
#include "datapod/core/mode.hpp"
#include "datapod/core/verify.hpp"
#include "datapod/matrix/dynamic.hpp"
#include "datapod/matrix/matrix.hpp"
#include "datapod/matrix/tensor.hpp"
#include "datapod/matrix/vector.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/sequential/array.hpp"
#include "datapod/sequential/string.hpp"
#include "datapod/sequential/vector.hpp"
#include "datapod/serialization/buf.hpp"
#include "datapod/serialization/serialized_size.hpp"
#include "datapod/type_hash/type_hash.hpp"

namespace datapod {

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
    template <typename T> struct is_container : std::false_type {};
    template <> struct is_container<String> : std::true_type {};
    template <typename T> struct is_container<Vector<T>> : std::true_type {};
    template <typename T> struct is_container<Optional<T>> : std::true_type {};
    template <typename A, typename B> struct is_container<Pair<A, B>> : std::true_type {};
    template <typename T, std::size_t N> struct is_container<Array<T, N>> : std::true_type {};
    template <typename... Ts> struct is_container<Tuple<Ts...>> : std::true_type {};
    template <typename... Ts> struct is_container<Variant<Ts...>> : std::true_type {};
    // Map and Set are type aliases for HashStorage
    template <typename T, template <typename> typename Ptr, typename GetKey, typename GetValue, typename Hash,
              typename Eq>
    struct is_container<HashStorage<T, Ptr, GetKey, GetValue, Hash, Eq>> : std::true_type {};
    // Heap-allocated mat types need special serialization
    template <typename T, std::size_t N> struct is_container<mat::vector<T, N, true>> : std::true_type {};
    template <typename T, std::size_t R, std::size_t C>
    struct is_container<mat::matrix<T, R, C, true>> : std::true_type {};
    template <typename T, std::size_t... Dims> struct is_container<mat::heap_tensor<T, Dims...>> : std::true_type {};
    // Dynamic mat types need special serialization (vector<T, Dynamic>, matrix<T, Dynamic, Dynamic>, dynamic_tensor<T>)
    template <typename T> struct is_container<mat::vector<T, mat::Dynamic, false>> : std::true_type {};
    template <typename T> struct is_container<mat::matrix<T, mat::Dynamic, mat::Dynamic, false>> : std::true_type {};
    template <typename T> struct is_container<mat::dynamic_tensor<T>> : std::true_type {};
    template <typename T> constexpr bool is_container_v = is_container<decay_t<T>>::value;

    // Serialize aggregate types (structs) using reflection
    template <Mode M, typename Ctx, typename T>
    std::enable_if_t<std::is_class_v<T> && !std::is_scalar_v<T> && !is_container_v<T>> //
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

    // Serialize HashStorage (used by Map and Set)
    template <Mode M, typename Ctx, typename T, template <typename> typename Ptr, typename GetKey, typename GetValue,
              typename Hash, typename Eq>
    void serialize(Ctx &ctx, HashStorage<T, Ptr, GetKey, GetValue, Hash, Eq> &value) {
        // Write size
        auto const sz = value.size();
        serialize<M>(ctx, const_cast<std::size_t &>(sz));

        // Write all entries
        for (auto &entry : value) {
            serialize<M>(ctx, const_cast<T &>(entry));
        }
    }

    // =============================================================================
    // Serialize heap-allocated mat types
    // =============================================================================

    // Serialize heap-allocated mat::vector
    template <Mode M, typename Ctx, typename T, std::size_t N>
    void serialize(Ctx &ctx, mat::vector<T, N, true> &value) {
        // Fixed size, just write all elements
        for (std::size_t i = 0; i < N; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // Serialize heap-allocated mat::matrix
    template <Mode M, typename Ctx, typename T, std::size_t R, std::size_t C>
    void serialize(Ctx &ctx, mat::matrix<T, R, C, true> &value) {
        // Fixed size, just write all elements (column-major order)
        for (std::size_t i = 0; i < R * C; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // Serialize heap-allocated mat::heap_tensor
    template <Mode M, typename Ctx, typename T, std::size_t... Dims>
    void serialize(Ctx &ctx, mat::heap_tensor<T, Dims...> &value) {
        // Fixed size, just write all elements
        constexpr std::size_t total = (Dims * ...);
        for (std::size_t i = 0; i < total; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // =============================================================================
    // Serialize dynamic mat types
    // =============================================================================

    // Serialize mat::vector<T, Dynamic>
    // Format: [size_t size][T data[0]][T data[1]]...[T data[size-1]]
    template <Mode M, typename Ctx, typename T> void serialize(Ctx &ctx, mat::vector<T, mat::Dynamic, false> &value) {
        // Write size
        auto const sz = value.size();
        serialize<M>(ctx, const_cast<std::size_t &>(sz));

        // Write elements
        for (std::size_t i = 0; i < sz; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // Serialize mat::matrix<T, Dynamic, Dynamic>
    // Format: [size_t rows][size_t cols][T data[0]]...[T data[rows*cols-1]]
    template <Mode M, typename Ctx, typename T>
    void serialize(Ctx &ctx, mat::matrix<T, mat::Dynamic, mat::Dynamic, false> &value) {
        // Write dimensions
        auto const rows = value.rows();
        auto const cols = value.cols();
        serialize<M>(ctx, const_cast<std::size_t &>(rows));
        serialize<M>(ctx, const_cast<std::size_t &>(cols));

        // Write elements (column-major order)
        std::size_t total = rows * cols;
        for (std::size_t i = 0; i < total; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // Serialize mat::dynamic_tensor
    // Format: [size_t rank][size_t dim0]...[size_t dimN][T data[...]]
    template <Mode M, typename Ctx, typename T> void serialize(Ctx &ctx, mat::dynamic_tensor<T> &value) {
        // Write rank (number of dimensions)
        auto const r = value.rank();
        serialize<M>(ctx, const_cast<std::size_t &>(r));

        // Write each dimension
        for (std::size_t i = 0; i < r; ++i) {
            auto const d = value.dim(i);
            serialize<M>(ctx, const_cast<std::size_t &>(d));
        }

        // Write elements (column-major order)
        std::size_t total = value.size();
        for (std::size_t i = 0; i < total; ++i) {
            serialize<M>(ctx, value[i]);
        }
    }

    // =============================================================================
    // Main serialize entry point
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> ByteBuf serialize(T &el) {
        auto b = Buf{};
        auto ctx = SerializationContext<Buf<ByteBuf>, M>{b};

        // Write integrity checksum placeholder if requested
        offset_t integrity_offset = 0;
        if constexpr (is_mode_enabled(M, Mode::WITH_INTEGRITY)) {
            hash_t placeholder = 0;
            integrity_offset = ctx.write(&placeholder, sizeof(hash_t), alignof(hash_t));
        }

        // Write version hash if requested
        if constexpr (is_mode_enabled(M, Mode::WITH_VERSION)) {
            auto const h = convert_endian<M>(type_hash<decay_t<T>>());
            ctx.write(&h, sizeof(h), alignof(hash_t));
        }

        serialize<M>(ctx, el);

        // Calculate and write integrity checksum
        if constexpr (is_mode_enabled(M, Mode::WITH_INTEGRITY)) {
            auto const checksum_start = integrity_offset + static_cast<offset_t>(sizeof(hash_t));
            auto const csum = b.checksum(checksum_start);
            auto const csum_converted = convert_endian<M>(csum);
            ctx.write(static_cast<std::size_t>(integrity_offset), csum_converted);
        }

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
    std::enable_if_t<std::is_class_v<T> && !std::is_scalar_v<T> && !is_container_v<T>> //
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

    // Deserialize HashStorage (used by Map and Set)
    template <Mode M, typename Ctx, typename T, template <typename> typename Ptr, typename GetKey, typename GetValue,
              typename Hash, typename Eq>
    void deserialize(Ctx &ctx, HashStorage<T, Ptr, GetKey, GetValue, Hash, Eq> &value) {
        // Read size
        std::size_t sz = 0;
        deserialize<M>(ctx, sz);

        // Clear existing entries
        value.clear();

        // Read all entries
        for (std::size_t i = 0; i < sz; ++i) {
            T entry;
            deserialize<M>(ctx, entry);
            value.insert(std::move(entry));
        }
    }

    // =============================================================================
    // Deserialize heap-allocated mat types
    // =============================================================================

    // Deserialize heap-allocated mat::vector
    template <Mode M, typename Ctx, typename T, std::size_t N>
    void deserialize(Ctx &ctx, mat::vector<T, N, true> &value) {
        // Fixed size, read all elements
        for (std::size_t i = 0; i < N; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // Deserialize heap-allocated mat::matrix
    template <Mode M, typename Ctx, typename T, std::size_t R, std::size_t C>
    void deserialize(Ctx &ctx, mat::matrix<T, R, C, true> &value) {
        // Fixed size, read all elements (column-major order)
        for (std::size_t i = 0; i < R * C; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // Deserialize heap-allocated mat::heap_tensor
    template <Mode M, typename Ctx, typename T, std::size_t... Dims>
    void deserialize(Ctx &ctx, mat::heap_tensor<T, Dims...> &value) {
        // Fixed size, read all elements
        constexpr std::size_t total = (Dims * ...);
        for (std::size_t i = 0; i < total; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // =============================================================================
    // Deserialize dynamic mat types
    // =============================================================================

    // Deserialize mat::vector<T, Dynamic>
    template <Mode M, typename Ctx, typename T> void deserialize(Ctx &ctx, mat::vector<T, mat::Dynamic, false> &value) {
        // Read size
        std::size_t sz = 0;
        deserialize<M>(ctx, sz);

        // Resize and read elements
        value.resize(sz);
        for (std::size_t i = 0; i < sz; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // Deserialize mat::matrix<T, Dynamic, Dynamic>
    template <Mode M, typename Ctx, typename T>
    void deserialize(Ctx &ctx, mat::matrix<T, mat::Dynamic, mat::Dynamic, false> &value) {
        // Read dimensions
        std::size_t rows = 0, cols = 0;
        deserialize<M>(ctx, rows);
        deserialize<M>(ctx, cols);

        // Resize and read elements
        value.resize(rows, cols);
        std::size_t total = rows * cols;
        for (std::size_t i = 0; i < total; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // Deserialize mat::dynamic_tensor
    template <Mode M, typename Ctx, typename T> void deserialize(Ctx &ctx, mat::dynamic_tensor<T> &value) {
        // Read rank
        std::size_t r = 0;
        deserialize<M>(ctx, r);

        // Read dimensions
        Vector<std::size_t> dims;
        dims.resize(r);
        for (std::size_t i = 0; i < r; ++i) {
            deserialize<M>(ctx, dims[i]);
        }

        // Resize and read elements
        value.resize(dims);
        std::size_t total = value.size();
        for (std::size_t i = 0; i < total; ++i) {
            deserialize<M>(ctx, value[i]);
        }
    }

    // =============================================================================
    // Main deserialize entry point
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> T deserialize(ByteBuf const &buf) {
        T result{};
        auto ctx = DeserializationContext<M>{buf.data(), buf.size()};

        // Verify integrity checksum if present
        if constexpr (is_mode_enabled(M, Mode::WITH_INTEGRITY)) {
            hash_t stored_checksum;
            ctx.align(alignof(hash_t));
            auto const checksum_start = ctx.pos_ + sizeof(hash_t);
            ctx.read(&stored_checksum, sizeof(hash_t));

            // Calculate checksum of data after the stored checksum
            auto const calculated_checksum = hash(std::string_view{
                reinterpret_cast<char const *>(buf.data() + checksum_start), buf.size() - checksum_start});

            verify(convert_endian<M>(stored_checksum) == calculated_checksum, "integrity check failed: data corrupted");
        }

        // Verify version hash if present
        if constexpr (is_mode_enabled(M, Mode::WITH_VERSION)) {
            hash_t stored_hash;
            ctx.align(alignof(hash_t));
            ctx.read(&stored_hash, sizeof(hash_t));
            auto const expected_hash = type_hash<decay_t<T>>();
            verify(convert_endian<M>(stored_hash) == expected_hash, "version mismatch: type schema changed");
        }

        deserialize<M>(ctx, result);
        return result;
    }

    template <Mode M = Mode::NONE, typename T> T deserialize(std::uint8_t const *data, std::size_t size) {
        T result{};
        auto ctx = DeserializationContext<M>{data, size};

        // Verify integrity checksum if present
        if constexpr (is_mode_enabled(M, Mode::WITH_INTEGRITY)) {
            hash_t stored_checksum;
            ctx.align(alignof(hash_t));
            auto const checksum_start = ctx.pos_ + sizeof(hash_t);
            ctx.read(&stored_checksum, sizeof(hash_t));

            // Calculate checksum of data after the stored checksum
            auto const calculated_checksum =
                hash(std::string_view{reinterpret_cast<char const *>(data + checksum_start), size - checksum_start});

            verify(convert_endian<M>(stored_checksum) == calculated_checksum, "integrity check failed: data corrupted");
        }

        // Verify version hash if present
        if constexpr (is_mode_enabled(M, Mode::WITH_VERSION)) {
            hash_t stored_hash;
            ctx.align(alignof(hash_t));
            ctx.read(&stored_hash, sizeof(hash_t));
            auto const expected_hash = type_hash<decay_t<T>>();
            verify(convert_endian<M>(stored_hash) == expected_hash, "version mismatch: type schema changed");
        }

        deserialize<M>(ctx, result);
        return result;
    }

    // =============================================================================
    // std::string_view overload
    // =============================================================================

    template <Mode M = Mode::NONE, typename T> T deserialize(std::string_view buf) {
        return deserialize<M, T>(reinterpret_cast<std::uint8_t const *>(buf.data()), buf.size());
    }

    // Overload for deserialize with explicit type parameter (alternative syntax)
    template <Mode M = Mode::NONE, typename T> void deserialize(ByteBuf const &buf, T &result) {
        auto ctx = DeserializationContext<M>{buf.data(), buf.size()};

        // Verify integrity checksum if present
        if constexpr (is_mode_enabled(M, Mode::WITH_INTEGRITY)) {
            hash_t stored_checksum;
            ctx.align(alignof(hash_t));
            auto const checksum_start = ctx.pos_ + sizeof(hash_t);
            ctx.read(&stored_checksum, sizeof(hash_t));

            // Calculate checksum of data after the stored checksum
            auto const calculated_checksum = hash(std::string_view{
                reinterpret_cast<char const *>(buf.data() + checksum_start), buf.size() - checksum_start});

            verify(convert_endian<M>(stored_checksum) == calculated_checksum, "integrity check failed: data corrupted");
        }

        // Verify version hash if present
        if constexpr (is_mode_enabled(M, Mode::WITH_VERSION)) {
            hash_t stored_hash;
            ctx.align(alignof(hash_t));
            ctx.read(&stored_hash, sizeof(hash_t));
            auto const expected_hash = type_hash<decay_t<T>>();
            verify(convert_endian<M>(stored_hash) == expected_hash, "version mismatch: type schema changed");
        }

        deserialize<M>(ctx, result);
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

} // namespace datapod
