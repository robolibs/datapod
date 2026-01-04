#pragma once

#include <memory>
#include <utility>

namespace datapod {

    /// Cow<T> - Clone-on-Write smart pointer
    /// Similar to Rust's Cow (Clone on Write)
    /// Holds either a borrowed reference or an owned value
    /// Clones only when mutation is needed
    /// Optimizes for read-heavy scenarios
    template <typename T> class Cow {
      public:
        /// Construct from borrowed reference
        static Cow borrowed(T const &value) {
            Cow cow;
            cow.borrowed_ = &value;
            cow.owned_ = nullptr;
            return cow;
        }

        /// Construct from owned value (moved)
        static Cow owned(T &&value) {
            Cow cow;
            cow.borrowed_ = nullptr;
            cow.owned_ = std::make_unique<T>(std::move(value));
            return cow;
        }

        /// Construct from owned value (copied)
        static Cow owned(T const &value) {
            Cow cow;
            cow.borrowed_ = nullptr;
            cow.owned_ = std::make_unique<T>(value);
            return cow;
        }

        /// Default constructor (empty)
        Cow() : borrowed_(nullptr), owned_(nullptr) {}

        /// Copy constructor - shares borrowed or clones owned
        Cow(Cow const &other) {
            if (other.is_borrowed()) {
                borrowed_ = other.borrowed_;
                owned_ = nullptr;
            } else if (other.is_owned()) {
                borrowed_ = nullptr;
                owned_ = std::make_unique<T>(*other.owned_);
            } else {
                borrowed_ = nullptr;
                owned_ = nullptr;
            }
        }

        /// Move constructor
        Cow(Cow &&other) noexcept : borrowed_(other.borrowed_), owned_(std::move(other.owned_)) {
            other.borrowed_ = nullptr;
        }

        /// Copy assignment
        Cow &operator=(Cow const &other) {
            if (this != &other) {
                if (other.is_borrowed()) {
                    borrowed_ = other.borrowed_;
                    owned_ = nullptr;
                } else if (other.is_owned()) {
                    borrowed_ = nullptr;
                    owned_ = std::make_unique<T>(*other.owned_);
                } else {
                    borrowed_ = nullptr;
                    owned_ = nullptr;
                }
            }
            return *this;
        }

        /// Move assignment
        Cow &operator=(Cow &&other) noexcept {
            if (this != &other) {
                borrowed_ = other.borrowed_;
                owned_ = std::move(other.owned_);
                other.borrowed_ = nullptr;
            }
            return *this;
        }

        ~Cow() = default;

        /// Check if holding a borrowed reference
        bool is_borrowed() const noexcept { return borrowed_ != nullptr; }

        /// Check if holding an owned value
        bool is_owned() const noexcept { return owned_ != nullptr; }

        /// Check if empty
        bool is_empty() const noexcept { return !is_borrowed() && !is_owned(); }

        /// Get const reference (no cloning)
        T const &operator*() const {
            if (is_borrowed()) {
                return *borrowed_;
            }
            if (is_owned()) {
                return *owned_;
            }
            throw std::runtime_error("Cow::operator* called on empty Cow");
        }

        /// Get const pointer (no cloning)
        T const *operator->() const {
            if (is_borrowed()) {
                return borrowed_;
            }
            if (is_owned()) {
                return owned_.get();
            }
            throw std::runtime_error("Cow::operator-> called on empty Cow");
        }

        /// Get const reference (no cloning)
        T const &get() const { return **this; }

        /// Get mutable reference (clones if borrowed)
        T &to_mut() {
            if (is_borrowed()) {
                // Clone the borrowed value
                owned_ = std::make_unique<T>(*borrowed_);
                borrowed_ = nullptr;
            }
            if (is_owned()) {
                return *owned_;
            }
            throw std::runtime_error("Cow::to_mut called on empty Cow");
        }

        /// Convert to owned value (clones if borrowed)
        Cow &make_owned() {
            if (is_borrowed()) {
                owned_ = std::make_unique<T>(*borrowed_);
                borrowed_ = nullptr;
            }
            return *this;
        }

        /// Take ownership of the value (clones if borrowed)
        T into_owned() && {
            if (is_borrowed()) {
                return T(*borrowed_);
            }
            if (is_owned()) {
                return std::move(*owned_);
            }
            throw std::runtime_error("Cow::into_owned called on empty Cow");
        }

        /// Clone the Cow (always creates owned copy)
        Cow clone() const {
            if (is_borrowed()) {
                return Cow::owned(*borrowed_);
            }
            if (is_owned()) {
                return Cow::owned(*owned_);
            }
            return Cow();
        }

        /// Comparison operators
        friend bool operator==(Cow const &a, Cow const &b) {
            if (a.is_empty() && b.is_empty())
                return true;
            if (a.is_empty() || b.is_empty())
                return false;
            return *a == *b;
        }

        friend bool operator!=(Cow const &a, Cow const &b) { return !(a == b); }

        friend bool operator<(Cow const &a, Cow const &b) {
            if (a.is_empty())
                return !b.is_empty();
            if (b.is_empty())
                return false;
            return *a < *b;
        }

        friend bool operator<=(Cow const &a, Cow const &b) { return !(b < a); }

        friend bool operator>(Cow const &a, Cow const &b) { return b < a; }

        friend bool operator>=(Cow const &a, Cow const &b) { return !(a < b); }

      private:
        T const *borrowed_;
        std::unique_ptr<T> owned_;
    };

} // namespace datapod
