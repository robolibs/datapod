#pragma once

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <iterator>

#include "datapod/containers/vector.hpp"
#include "datapod/core/verify.hpp"

namespace datapod {

    template <typename Vec> struct BaseFlatMatrix {
        using value_type = typename Vec::value_type;
        using size_type = typename Vec::size_type;

        struct Row {
            Row(BaseFlatMatrix &matrix, size_type const i) : matrix_(matrix), i_(i) {}

            using iterator = typename Vec::iterator;
            using const_iterator = typename Vec::const_iterator;

            const_iterator begin() const { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * i_); }
            const_iterator end() const { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * (i_ + 1)); }
            iterator begin() { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * i_); }
            iterator end() { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * (i_ + 1)); }
            friend const_iterator begin(Row const &r) { return r.begin(); }
            friend const_iterator end(Row const &r) { return r.end(); }
            friend iterator begin(Row &r) { return r.begin(); }
            friend iterator end(Row &r) { return r.end(); }

            value_type &operator[](size_type const j) {
                assert(j < matrix_.n_columns_);
                auto const pos = matrix_.n_columns_ * i_ + j;
                return matrix_.entries_[pos];
            }

            BaseFlatMatrix &matrix_;
            size_type i_;
        };

        struct ConstRow {
            ConstRow(BaseFlatMatrix const &matrix, size_type const i) : matrix_(matrix), i_(i) {}

            using iterator = typename Vec::const_iterator;

            iterator begin() const { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * i_); }
            iterator end() const { return std::next(matrix_.entries_.begin(), matrix_.n_columns_ * (i_ + 1)); }
            friend iterator begin(ConstRow const &r) { return r.begin(); }
            friend iterator end(ConstRow const &r) { return r.end(); }

            value_type const &operator[](size_type const j) const {
                assert(j < matrix_.n_columns_);
                auto const pos = matrix_.n_columns_ * i_ + j;
                return matrix_.entries_[pos];
            }

            BaseFlatMatrix const &matrix_;
            size_type i_;
        };

        Row operator[](size_type i) {
            assert(i < n_rows_);
            return {*this, i};
        }
        ConstRow operator[](size_type i) const {
            assert(i < n_rows_);
            return {*this, i};
        }

        value_type &operator()(size_type const i, size_type const j) {
            assert(i < n_rows_ && j < n_columns_);
            return entries_[n_columns_ * i + j];
        }

        Row at(size_type const i) {
            verify(i < n_rows_, "matrix::at: index out of range");
            return {*this, i};
        }

        ConstRow at(size_type const i) const {
            verify(i < n_rows_, "matrix::at: index out of range");
            return {*this, i};
        }

        void resize(size_type const n_rows, size_type const n_columns) {
            n_rows_ = n_rows;
            n_columns_ = n_columns;
            entries_.resize(n_rows * n_columns);
        }

        void reset(value_type const &t) { std::fill(std::begin(entries_), std::end(entries_), t); }

        size_type n_rows_{0U}, n_columns_{0U};
        Vec entries_;
    };

    template <typename T> using FlatMatrix = BaseFlatMatrix<Vector<T>>;

    template <typename T>
    inline FlatMatrix<T> make_flat_matrix(std::uint32_t const n_rows, std::uint32_t const n_columns,
                                          T const &init = T{}) {
        auto v = Vector<T>{};
        v.resize(n_rows * n_columns, init);
        return {n_rows, n_columns, std::move(v)};
    }

} // namespace datapod
