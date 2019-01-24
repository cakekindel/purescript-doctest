///////////////////////////////////////////////////////////////////////////////
//
// Module      :  purescript.h
// Copyright   :  (c) Andy Arvanitis 2018
// License     :  BSD
//
// Maintainer  :  Andy Arvanitis
// Stability   :  experimental
// Portability :
//
// Basic types and functions to support purescript-to-C++ rendering
//
///////////////////////////////////////////////////////////////////////////////
//
#ifndef purescript_H
#define purescript_H

#include <memory>
#include <functional>
#include <deque>
#include <string>
#include <limits>
#include "string_literal_dict.h"


namespace purescript {

    using std::string;

    class boxed : public std::shared_ptr<void> {
        
    public:
        using fn_t = std::function<boxed(const boxed&)>;
        using eff_fn_t = std::function<boxed(void)>;
        using dict_t = string_literal_dict_t<boxed>;
        using array_t = std::deque<boxed>;

    public:
        using std::shared_ptr<void>::shared_ptr;

        boxed() noexcept : std::shared_ptr<void>() {}
        boxed(const std::nullptr_t) noexcept : std::shared_ptr<void>() {}
        boxed(const std::shared_ptr<void>& p) : std::shared_ptr<void>::shared_ptr(p) {}
        boxed(const int n) : std::shared_ptr<void>(std::make_shared<int>(n)) {}
        boxed(const long n) : std::shared_ptr<void>(std::make_shared<int>(static_cast<int>(n))) {
#if !defined(NDEBUG) // if debug build
            if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max()) {
                throw std::runtime_error("integer out of range");
            }
#endif // !defined(NDEBUG)
        }
        boxed(const unsigned long n) : std::shared_ptr<void>(std::make_shared<int>(static_cast<int>(n))) {
#if !defined(NDEBUG) // if debug build
            if (n > std::numeric_limits<int>::max()) {
                throw std::runtime_error("integer out of range");
            }
#endif // !defined(NDEBUG)
        }
        boxed(const double n) : std::shared_ptr<void>(std::make_shared<double>(n)) {}
        boxed(const bool b) : std::shared_ptr<void>(std::make_shared<bool>(b)) {}
        boxed(const char s[]) : std::shared_ptr<void>(std::make_shared<string>(s)) {}
        boxed(string&& s) : std::shared_ptr<void>(std::make_shared<string>(std::move(s))) {}
        boxed(const string& s) : std::shared_ptr<void>(std::make_shared<string>(s)) {}
        boxed(array_t&& l) : std::shared_ptr<void>(std::make_shared<array_t>(std::move(l))) {}
        boxed(const array_t& l) : std::shared_ptr<void>(std::make_shared<array_t>(l)) {}
        boxed(dict_t&& m) : std::shared_ptr<void>(std::make_shared<dict_t>(std::move(m))) {}
        boxed(const dict_t& m) : std::shared_ptr<void>(std::make_shared<dict_t>(m)) {}

        template <typename T,
                  typename = typename std::enable_if<!std::is_same<boxed,T>::value>::type>
        boxed(const T& f,
              typename std::enable_if<std::is_assignable<std::function<boxed(boxed)>,T>::value>::type* = 0)
              : std::shared_ptr<void>(std::make_shared<fn_t>(f)) {
        }

        template <typename T,
                  typename = typename std::enable_if<!std::is_same<boxed,T>::value>::type>
        boxed(const T& f,
              typename std::enable_if<std::is_assignable<std::function<boxed(void)>,T>::value>::type* = 0)
              : std::shared_ptr<void>(std::make_shared<eff_fn_t>(f)) {
        }

        auto operator()(const boxed& arg) const -> boxed {
            auto& f = *static_cast<fn_t*>(get());
            return f(arg);
        }

        auto operator()() const -> boxed {
            auto& f = *static_cast<eff_fn_t*>(get());
            return f();
        }

        auto operator[](const char key[]) const -> const boxed& {
          return (*static_cast<const dict_t*>(get()))[key];
        }

        auto operator[](const char key[]) -> boxed& {
          return (*static_cast<dict_t*>(get()))[key];
        }

#if !defined(NDEBUG) // if debug build
        auto operator[](const int index) const -> const boxed& {
            return static_cast<const array_t*>(get())->at(index);
        }

        auto operator[](const int index) -> boxed& {
            return static_cast<array_t*>(get())->at(index);
        }
#else  // not debug build
        auto operator[](const int index) const -> const boxed& {
            return (*static_cast<const array_t*>(get()))[index];
        }

        auto operator[](const int index) -> boxed& {
            return (*static_cast<array_t*>(get()))[index];
        }
#endif // !defined(NDEBUG)

        class weak {
            std::weak_ptr<void> ptr;

            public:
            weak() = delete;
            weak(boxed& b) : ptr(b) {}

            operator boxed() const {
#if defined(NDEBUG)
                return ptr.lock();
#else
                return static_cast<std::shared_ptr<void>>(ptr);
#endif
            }

        }; // class weak

    }; // class boxed

    using fn_t = boxed::fn_t;
    using eff_fn_t = boxed::eff_fn_t;
    using dict_t = boxed::dict_t;
    using array_t = boxed::array_t;

    template <typename T, typename... Args>
    inline auto box(Args&&... args) -> boxed {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    constexpr auto unbox(const boxed& b) -> const T& {
        return *static_cast<const T*>(b.get());
    }

    template <typename T>
    constexpr auto unbox(boxed& b) -> T& {
        return *static_cast<T*>(b.get());
    }

    template <typename T>
    constexpr auto unbox(const T value) -> T {
        return value;
    }

    template <typename T,
              typename = typename std::enable_if<std::is_same<T, int>::value>::type>
    constexpr auto unbox(const std::size_t value) -> long long {
        return value;
    }

    class boxed_r {
        friend class boxed_weak_r;
        std::shared_ptr<boxed> ptr;
        std::shared_ptr<boxed::weak> wptr;

        public:
        boxed_r() : ptr(std::make_shared<boxed>())
                  , wptr(std::make_shared<boxed::weak>(*ptr)) {}

        operator const boxed&() const {
            return *ptr;
        }

        operator boxed&() {
            return *ptr;
        }

        auto operator()() const -> boxed {
            return (*ptr)();
        }

        auto operator()(const boxed& arg) const -> boxed {
            return (*ptr)(arg);
        }

        template <typename T>
        auto operator=(T&& right) -> boxed_r& {
            *ptr = std::forward<T>(right);
            *wptr = *ptr;
            return *this;
        }

    }; // class boxed_r

    class boxed_weak_r {
        std::shared_ptr<boxed::weak> wptr;

    public:
        boxed_weak_r(const boxed_r& r) : wptr(r.wptr) {}

        operator boxed() const {
            return *wptr;
        }

        auto operator()() const -> boxed {
            return static_cast<boxed>(*wptr)();
        }

        auto operator()(const boxed& arg) const -> boxed {
            return static_cast<boxed>(*wptr)(arg);
        }

    }; // class boxed_weak_r

    inline auto array_length(const boxed& a) -> boxed::array_t::size_type {
        return unbox<boxed::array_t>(a).size();
    }

    constexpr auto undefined = nullptr;

} // namespace purescript

#define DEFINE_FOREIGN_DICTIONARY_AND_ACCESSOR() \
    inline auto foreign() -> dict_t& {\
        static dict_t $dict$;\
        return $dict$;\
    }

#define FOREIGN_BEGIN(NS) namespace NS {\
    using namespace purescript;\
    DEFINE_FOREIGN_DICTIONARY_AND_ACCESSOR()\
    static const auto $foreign_exports_init$ = ([]() -> char {\
        dict_t& exports = foreign();
#define FOREIGN_END return 0; }()); }

#endif // purescript_H
