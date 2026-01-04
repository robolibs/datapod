#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "datapod/pods/memory/allocator.hpp"
#include "datapod/pods/sequential/vector.hpp"
#include "datapod/serialization/serialize.hpp"

#include "datapod/serialization/serialize.hpp"

namespace datapod {

    template <typename T, std::size_t InlineCapacity, typename Alloc = Allocator<T>> class Vectra {
      public:
        using value_type = T;
        using allocator_type = Alloc;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = T const &;
        using pointer = T *;
        using const_pointer = T const *;
        using iterator = T *;
        using const_iterator = T const *;

        static constexpr size_type inline_capacity = InlineCapacity;

        Vectra() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
            : alloc_{}, data_{InlineCapacity > 0 ? inline_data() : nullptr}, size_{0}, capacity_{InlineCapacity},
              serialization_cache_{}, snapshot_valid_{false} {}

        explicit Vectra(size_type count) : Vectra() { assign(count, value_type{}); }

        Vectra(size_type count, value_type const &value) : Vectra() { assign(count, value); }

        Vectra(std::initializer_list<value_type> init) : Vectra() { assign(init.begin(), init.end()); }

        Vectra(Vectra const &other) : Vectra() { assign(other.begin(), other.end()); }

        Vectra(Vectra &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>)
            : alloc_{std::move(other.alloc_)}, data_{InlineCapacity > 0 ? inline_data() : nullptr}, size_{0},
              capacity_{InlineCapacity}, serialization_cache_{}, snapshot_valid_{false} {
            move_from(std::move(other));
        }

        ~Vectra() {
            clear();
            if (!using_inline_storage() && data_ != nullptr) {
                deallocate_heap(data_, capacity_);
            }
        }

        Vectra &operator=(Vectra const &other) {
            if (this != &other) {
                assign(other.begin(), other.end());
            }
            return *this;
        }

        Vectra &operator=(Vectra &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>) {
            if (this != &other) {
                clear();
                if (!using_inline_storage() && data_ != nullptr) {
                    deallocate_heap(data_, capacity_);
                }
                data_ = InlineCapacity > 0 ? inline_data() : nullptr;
                capacity_ = InlineCapacity;
                size_ = 0;
                move_from(std::move(other));
                snapshot_valid_ = false;
            }
            return *this;
        }

        Vectra &operator=(std::initializer_list<value_type> init) {
            assign(init.begin(), init.end());
            return *this;
        }

        reference operator[](size_type index) noexcept { return data_[index]; }
        const_reference operator[](size_type index) const noexcept { return data_[index]; }

        reference at(size_type index) {
            if (index >= size_) {
                throw std::out_of_range("Vectra::at: index out of range");
            }
            return data_[index];
        }

        const_reference at(size_type index) const {
            if (index >= size_) {
                throw std::out_of_range("Vectra::at: index out of range");
            }
            return data_[index];
        }

        pointer data() noexcept { return data_; }
        const_pointer data() const noexcept { return data_; }

        iterator begin() noexcept { return data_; }
        const_iterator begin() const noexcept { return data_; }
        const_iterator cbegin() const noexcept { return data_; }

        iterator end() noexcept { return data_ + size_; }
        const_iterator end() const noexcept { return data_ + size_; }
        const_iterator cend() const noexcept { return data_ + size_; }

        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }
        size_type capacity() const noexcept { return capacity_; }

        bool using_inline_storage() const noexcept {
            if constexpr (InlineCapacity == 0) {
                return false;
            } else {
                return data_ == inline_data();
            }
        }

        void clear() noexcept {
            destroy_range(data_, data_ + size_);
            size_ = 0;
            mark_dirty();
        }

        void reserve(size_type new_cap) {
            if (new_cap <= capacity_) {
                return;
            }
            ensure_capacity(new_cap);
        }

        void shrink_to_fit() {
            if (size_ == capacity_) {
                return;
            }
            if constexpr (InlineCapacity > 0) {
                if (size_ <= InlineCapacity) {
                    reallocate_to_inline();
                    return;
                }
            }
            reallocate_to(size_);
        }

        void resize(size_type count) {
            if (count < size_) {
                destroy_range(data_ + count, data_ + size_);
                size_ = count;
                mark_dirty();
                return;
            }
            reserve(count);
            auto tail = data_ + size_;
            for (; size_ < count; ++size_, ++tail) {
                construct_at(tail);
            }
            mark_dirty();
        }

        void resize(size_type count, value_type const &value) {
            if (count < size_) {
                destroy_range(data_ + count, data_ + size_);
                size_ = count;
                mark_dirty();
                return;
            }
            reserve(count);
            auto tail = data_ + size_;
            for (; size_ < count; ++size_, ++tail) {
                construct_at(tail, value);
            }
            mark_dirty();
        }

        void push_back(value_type const &value) {
            ensure_capacity(size_ + 1);
            construct_at(data_ + size_, value);
            ++size_;
            mark_dirty();
        }

        void push_back(value_type &&value) {
            ensure_capacity(size_ + 1);
            construct_at(data_ + size_, std::move(value));
            ++size_;
            mark_dirty();
        }

        template <typename... Args> reference emplace_back(Args &&...args) {
            ensure_capacity(size_ + 1);
            construct_at(data_ + size_, std::forward<Args>(args)...);
            ++size_;
            mark_dirty();
            return back();
        }

        void pop_back() noexcept {
            if (size_ == 0) {
                return;
            }
            --size_;
            destroy_at(data_ + size_);
            mark_dirty();
        }

        reference front() noexcept { return data_[0]; }
        const_reference front() const noexcept { return data_[0]; }

        reference back() noexcept { return data_[size_ - 1]; }
        const_reference back() const noexcept { return data_[size_ - 1]; }

        void assign(size_type count, value_type const &value) {
            clear();
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                construct_at(data_ + size_, value);
                ++size_;
            }
            mark_dirty();
        }

        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        void assign(InputIt first, InputIt last) {
            clear();
            auto dist = std::distance(first, last);
            if (dist > 0) {
                reserve(static_cast<size_type>(dist));
            }
            for (; first != last; ++first) {
                construct_at(data_ + size_, *first);
                ++size_;
            }
            mark_dirty();
        }

        void swap(Vectra &other) noexcept(std::is_nothrow_move_constructible_v<value_type>) {
            if (this == &other) {
                return;
            }
            Vectra tmp = std::move(other);
            other = std::move(*this);
            *this = std::move(tmp);
        }

        auto members() {
            ensure_snapshot();
            return std::tie(serialization_cache_);
        }

        auto members() const {
            ensure_snapshot();
            return std::tie(serialization_cache_);
        }

        void rebuild_from_snapshot() {
            clear_internal();
            auto snapshot_size = serialization_cache_.size();
            if (snapshot_size > 0) {
                reserve(snapshot_size);
                pointer dest = data_;
                for (auto &element : serialization_cache_) {
                    construct_at(dest, element);
                    ++dest;
                    ++size_;
                }
            }
            snapshot_valid_ = true;
        }

      private:
        using traits = std::allocator_traits<allocator_type>;
        using storage_type = std::aligned_storage_t<sizeof(T), alignof(T)>;

        pointer inline_data() noexcept {
            if constexpr (InlineCapacity == 0) {
                return nullptr;
            } else {
                return reinterpret_cast<pointer>(inline_storage_);
            }
        }

        const_pointer inline_data() const noexcept {
            if constexpr (InlineCapacity == 0) {
                return nullptr;
            } else {
                return reinterpret_cast<const_pointer>(inline_storage_);
            }
        }

        void construct_at(pointer ptr) { traits::construct(alloc_, ptr); }

        template <typename... Args> void construct_at(pointer ptr, Args &&...args) {
            traits::construct(alloc_, ptr, std::forward<Args>(args)...);
        }

        void destroy_at(pointer ptr) noexcept { traits::destroy(alloc_, ptr); }

        void destroy_range(pointer first, pointer last) noexcept {
            while (last != first) {
                --last;
                destroy_at(last);
            }
        }

        void mark_dirty() const noexcept { snapshot_valid_ = false; }

        void ensure_capacity(size_type min_capacity) {
            if (min_capacity <= capacity_) {
                return;
            }
            size_type new_capacity = std::max(min_capacity, compute_new_capacity(capacity_));
            if constexpr (InlineCapacity > 0) {
                if (new_capacity <= InlineCapacity) {
                    new_capacity = InlineCapacity;
                }
            }
            reallocate_to(new_capacity);
        }

        size_type compute_new_capacity(size_type current) const noexcept {
            if (current == 0) {
                return 1;
            }
            size_type grown = current + current / 2;
            if (grown < current) {
                grown = std::numeric_limits<size_type>::max();
            }
            return grown;
        }

        void reallocate_to(size_type new_capacity) {
            pointer target = nullptr;
            bool target_inline = false;

            if constexpr (InlineCapacity > 0) {
                if (new_capacity <= InlineCapacity) {
                    target = inline_data();
                    target_inline = true;
                    new_capacity = InlineCapacity;
                }
            }

            if (target == nullptr) {
                target = allocate_heap(new_capacity);
            }

            if (target == data_) {
                capacity_ = new_capacity;
                return;
            }

            pointer old_data = data_;
            size_type old_capacity = capacity_;
            bool was_inline = using_inline_storage();

            move_elements(target);

            if (old_data != nullptr && !was_inline) {
                deallocate_heap(old_data, old_capacity);
            }

            data_ = target;
            capacity_ = new_capacity;

            if constexpr (InlineCapacity > 0) {
                if (target_inline) {
                    // nothing else to do when moving back to inline storage
                }
            }
        }

        void reallocate_to_inline() {
            if constexpr (InlineCapacity == 0) {
                return;
            }
            if (using_inline_storage()) {
                return;
            }
            pointer target = inline_data();
            move_elements(target);
            deallocate_heap(data_, capacity_);
            data_ = target;
            capacity_ = InlineCapacity;
        }

        void move_elements(pointer target) {
            pointer dst = target;
            for (size_type i = 0; i < size_; ++i, ++dst) {
                construct_at(dst, std::move_if_noexcept(data_[i]));
            }
            destroy_range(data_, data_ + size_);
        }

        pointer allocate_heap(size_type count) { return count ? traits::allocate(alloc_, count) : nullptr; }

        void deallocate_heap(pointer ptr, size_type count) {
            if (ptr != nullptr && count > 0) {
                traits::deallocate(alloc_, ptr, count);
            }
        }

        void move_from(Vectra &&other) {
            if (other.using_inline_storage()) {
                reserve(other.size_);
                pointer dst = data_;
                for (size_type i = 0; i < other.size_; ++i, ++dst) {
                    construct_at(dst, std::move(other.data_[i]));
                }
                size_ = other.size_;
                other.clear_internal();
            } else {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = InlineCapacity > 0 ? other.inline_data() : nullptr;
                other.capacity_ = InlineCapacity;
                other.size_ = 0;
            }
            mark_dirty();
        }

        void ensure_snapshot() const {
            if (snapshot_valid_) {
                return;
            }
            serialization_cache_.clear();
            serialization_cache_.reserve(size_);
            for (size_type i = 0; i < size_; ++i) {
                serialization_cache_.push_back(data_[i]);
            }
            snapshot_valid_ = true;
        }

        void clear_internal() noexcept {
            destroy_range(data_, data_ + size_);
            size_ = 0;
        }

        allocator_type alloc_;
        pointer data_;
        size_type size_;
        size_type capacity_;
        storage_type inline_storage_[(InlineCapacity > 0) ? InlineCapacity : 1];
        mutable Vector<T> serialization_cache_;
        mutable bool snapshot_valid_;
    };

    template <typename T, std::size_t InlineCapacity, typename Alloc>
    void swap(Vectra<T, InlineCapacity, Alloc> &lhs,
              Vectra<T, InlineCapacity, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

    template <Mode M, typename Ctx, typename T, std::size_t InlineCapacity, typename Alloc>
    void serialize(Ctx &ctx, Vectra<T, InlineCapacity, Alloc> const &value) {
        auto size = value.size();
        serialize<M>(ctx, size);
        for (auto const &element : value) {
            serialize<M>(ctx, element);
        }
    }

    template <Mode M, typename Ctx, typename T, std::size_t InlineCapacity, typename Alloc>
    void deserialize(Ctx &ctx, Vectra<T, InlineCapacity, Alloc> &value) {
        using size_type = typename Vectra<T, InlineCapacity, Alloc>::size_type;
        size_type new_size{};
        deserialize<M>(ctx, new_size);
        value.clear();
        value.reserve(new_size);
        for (size_type i = 0; i < new_size; ++i) {
            T element{};
            deserialize<M>(ctx, element);
            value.push_back(std::move(element));
        }
    }

} // namespace datapod
