#pragma once

/**
 * @file dynamic.hpp
 * @brief Re-exports dynamic types and provides additional utilities
 *
 * This header is for convenience - it re-exports the dynamic specializations
 * from vector.hpp and matrix.hpp, plus provides the dynamic_tensor type.
 *
 * The Dynamic sentinel and specializations are defined in their respective files:
 *   - vector<T, Dynamic>  in vector.hpp
 *   - matrix<T, Dynamic, Dynamic>  in matrix.hpp
 *
 * For tensors, we provide a separate dynamic_tensor<T> class since tensors
 * have variadic dimension parameters and can't easily use the Dynamic sentinel.
 * This follows Eigen's approach where Tensor<T, Rank> has fixed rank.
 */

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

#include "datapod/core/aligned_alloc.hpp"
#include "datapod/matrix/matrix.hpp"     // For Dynamic, matrix<T, Dynamic, Dynamic>
#include "datapod/matrix/vector.hpp"     // For Dynamic, vector<T, Dynamic>
#include "datapod/sequential/vector.hpp" // For datapod::Vector (shape storage)

namespace datapod {
    namespace mat {

        // Dynamic sentinel is already defined in vector.hpp
        // Re-exported here for convenience

        // =============================================================================
        // DYNAMIC TENSOR (runtime-ranked, runtime-sized)
        // =============================================================================
        /**
         * @brief Runtime-ranked and runtime-sized tensor
         *
         * Unlike fixed-size tensor<T, Dims...>, this stores both rank and shape at runtime.
         * This follows Eigen's unsupported Tensor module approach.
         *
         * Always heap-allocated with SIMD alignment. Column-major storage.
         *
         * Serialization format: [size_t rank][size_t dim0]...[size_t dimN][T data[...]]
         *
         * Examples:
         *   dynamic_tensor<double> t({10, 20, 30});  // 10x20x30 tensor (rank 3)
         *   dynamic_tensor<float> cube({64, 64, 64}); // 64^3 cube
         *   t.resize({20, 30, 40});                   // Resize (can change rank too!)
         */
        template <typename T> struct dynamic_tensor {
            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr bool is_pod = false;
            static constexpr bool uses_heap = true;
            static constexpr bool is_dynamic = true;

          private:
            datapod::Vector<size_t> dims_;    // Shape (runtime rank)
            datapod::Vector<size_t> strides_; // Strides for indexing
            size_t size_;                     // Total elements
            T *data_;

            void compute_strides() {
                strides_.resize(dims_.size());
                if (dims_.empty())
                    return;

                // Column-major strides: stride[i] = product of dims[0..i-1]
                strides_[0] = 1;
                for (size_t i = 1; i < dims_.size(); ++i) {
                    strides_[i] = strides_[i - 1] * dims_[i - 1];
                }
            }

            void compute_size() {
                if (dims_.empty()) {
                    size_ = 0;
                    return;
                }
                size_ = 1;
                for (size_t d : dims_) {
                    size_ *= d;
                }
            }

            void allocate() {
                if (size_ > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                    for (size_t i = 0; i < size_; ++i) {
                        new (&data_[i]) T{};
                    }
                } else {
                    data_ = nullptr;
                }
            }

            void deallocate() {
                if (data_) {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                    data_ = nullptr;
                }
            }

          public:
            // Default constructor - empty tensor (rank 0)
            dynamic_tensor() noexcept : dims_(), strides_(), size_(0), data_(nullptr) {}

            // Shape constructor (initializer_list)
            explicit dynamic_tensor(std::initializer_list<size_t> shape)
                : dims_(), strides_(), size_(0), data_(nullptr) {
                dims_.reserve(shape.size());
                for (size_t d : shape) {
                    dims_.push_back(d);
                }
                compute_strides();
                compute_size();
                allocate();
            }

            // Shape constructor from vector
            explicit dynamic_tensor(const datapod::Vector<size_t> &shape)
                : dims_(shape), strides_(), size_(0), data_(nullptr) {
                compute_strides();
                compute_size();
                allocate();
            }

            // Copy constructor
            dynamic_tensor(const dynamic_tensor &other)
                : dims_(other.dims_), strides_(other.strides_), size_(other.size_), data_(nullptr) {
                if (size_ > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                    for (size_t i = 0; i < size_; ++i) {
                        new (&data_[i]) T(other.data_[i]);
                    }
                }
            }

            // Move constructor
            dynamic_tensor(dynamic_tensor &&other) noexcept
                : dims_(std::move(other.dims_)), strides_(std::move(other.strides_)), size_(other.size_),
                  data_(other.data_) {
                other.size_ = 0;
                other.data_ = nullptr;
            }

            // Destructor
            ~dynamic_tensor() { deallocate(); }

            // Copy assignment
            dynamic_tensor &operator=(const dynamic_tensor &other) {
                if (this != &other) {
                    deallocate();
                    dims_ = other.dims_;
                    strides_ = other.strides_;
                    size_ = other.size_;
                    if (size_ > 0) {
                        data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                        for (size_t i = 0; i < size_; ++i) {
                            new (&data_[i]) T(other.data_[i]);
                        }
                    }
                }
                return *this;
            }

            // Move assignment
            dynamic_tensor &operator=(dynamic_tensor &&other) noexcept {
                if (this != &other) {
                    deallocate();
                    dims_ = std::move(other.dims_);
                    strides_ = std::move(other.strides_);
                    size_ = other.size_;
                    data_ = other.data_;
                    other.size_ = 0;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // Multi-dimensional indexing via initializer_list
            reference operator()(std::initializer_list<size_t> indices) noexcept {
                size_t linear = 0;
                size_t i = 0;
                for (size_t idx : indices) {
                    linear += idx * strides_[i++];
                }
                return data_[linear];
            }

            const_reference operator()(std::initializer_list<size_t> indices) const noexcept {
                size_t linear = 0;
                size_t i = 0;
                for (size_t idx : indices) {
                    linear += idx * strides_[i++];
                }
                return data_[linear];
            }

            // 3D convenience accessor
            reference operator()(size_t i, size_t j, size_t k) noexcept {
                return data_[i * strides_[0] + j * strides_[1] + k * strides_[2]];
            }

            const_reference operator()(size_t i, size_t j, size_t k) const noexcept {
                return data_[i * strides_[0] + j * strides_[1] + k * strides_[2]];
            }

            // 4D convenience accessor
            reference operator()(size_t i, size_t j, size_t k, size_t l) noexcept {
                return data_[i * strides_[0] + j * strides_[1] + k * strides_[2] + l * strides_[3]];
            }

            const_reference operator()(size_t i, size_t j, size_t k, size_t l) const noexcept {
                return data_[i * strides_[0] + j * strides_[1] + k * strides_[2] + l * strides_[3]];
            }

            // Bounds-checked access
            reference at(std::initializer_list<size_t> indices) {
                if (indices.size() != dims_.size()) {
                    throw std::invalid_argument("dynamic_tensor::at: wrong number of indices");
                }
                size_t linear = 0;
                size_t i = 0;
                for (size_t idx : indices) {
                    if (idx >= dims_[i]) {
                        throw std::out_of_range("dynamic_tensor::at");
                    }
                    linear += idx * strides_[i++];
                }
                return data_[linear];
            }

            const_reference at(std::initializer_list<size_t> indices) const {
                if (indices.size() != dims_.size()) {
                    throw std::invalid_argument("dynamic_tensor::at: wrong number of indices");
                }
                size_t linear = 0;
                size_t i = 0;
                for (size_t idx : indices) {
                    if (idx >= dims_[i]) {
                        throw std::out_of_range("dynamic_tensor::at");
                    }
                    linear += idx * strides_[i++];
                }
                return data_[linear];
            }

            // 1D indexing (linear access)
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Dimensions
            size_type rank() const noexcept { return dims_.size(); }
            size_type size() const noexcept { return size_; }
            bool empty() const noexcept { return size_ == 0; }
            const datapod::Vector<size_t> &shape() const noexcept { return dims_; }
            const datapod::Vector<size_t> &strides() const noexcept { return strides_; }
            size_type dim(size_t i) const noexcept { return dims_[i]; }

            // Resize (destructive)
            void resize(std::initializer_list<size_t> new_shape) {
                deallocate();
                dims_.clear();
                dims_.reserve(new_shape.size());
                for (size_t d : new_shape) {
                    dims_.push_back(d);
                }
                compute_strides();
                compute_size();
                allocate();
            }

            void resize(const datapod::Vector<size_t> &new_shape) {
                deallocate();
                dims_ = new_shape;
                compute_strides();
                compute_size();
                allocate();
            }

            // Iterators
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + size_; }
            const_iterator end() const noexcept { return data_ + size_; }
            const_iterator cend() const noexcept { return data_ + size_; }

            // Operations
            void fill(const T &value) noexcept {
                for (size_t i = 0; i < size_; ++i) {
                    data_[i] = value;
                }
            }

            void setZero() { fill(T{}); }

            void swap(dynamic_tensor &other) noexcept {
                dims_.swap(other.dims_);
                strides_.swap(other.strides_);
                std::swap(size_, other.size_);
                std::swap(data_, other.data_);
            }

            // Comparison
            bool operator==(const dynamic_tensor &other) const noexcept {
                if (dims_.size() != other.dims_.size())
                    return false;
                for (size_t i = 0; i < dims_.size(); ++i) {
                    if (dims_[i] != other.dims_[i])
                        return false;
                }
                for (size_t i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i]))
                        return false;
                }
                return true;
            }

            bool operator!=(const dynamic_tensor &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // TYPE TRAITS
        // =============================================================================

        template <typename T> struct is_dynamic_tensor : std::false_type {};
        template <typename T> struct is_dynamic_tensor<dynamic_tensor<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_dynamic_tensor_v = is_dynamic_tensor<T>::value;

        // Check if any type is dynamic (vector, matrix, or tensor)
        template <typename T>
        inline constexpr bool is_dynamic_v = is_dynamic_vector_v<T> || is_dynamic_matrix_v<T> || is_dynamic_tensor_v<T>;

        // =============================================================================
        // EIGEN-STYLE ALIASES
        // =============================================================================

        // Dynamic tensors (no Eigen equivalent for runtime-rank, but useful)
        using TensorXf = dynamic_tensor<float>;
        using TensorXd = dynamic_tensor<double>;
        using TensorXi = dynamic_tensor<int>;

    } // namespace mat
} // namespace datapod
