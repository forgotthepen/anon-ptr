/*
MIT License

Copyright (c) 2025 forgotthepen (https://github.com/forgotthepen/anon-ptr)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <memory>
#include <utility> // std::forward, std::move
#include <typeinfo> // std::type_info, std::bad_cast
#include <type_traits> // std::decay, std::enable_if, std::is_same
#include <string>


namespace nonstd {
    class anon_ptr {
    private:
        template<typename Ty>
        using value_of = std::decay<Ty>;

        struct IAnon {
            virtual ~IAnon() = default;
            virtual void* obj_addr() noexcept = 0;
            virtual const std::type_info& obj_type() const noexcept = 0;
            virtual std::unique_ptr<IAnon> copy() const = 0;
        };

        template<typename Ty>
        class AnonImpl : public IAnon {
        private:
            using Tobj = typename value_of<Ty>::type;

            Tobj obj_;

        public:
            template<typename ...Args>
            AnonImpl(Args&& ...args):
                obj_( std::forward<Args>(args) ... )
            { }

            void* obj_addr() noexcept override {
                return reinterpret_cast<void *>(&obj_);
            }

            const std::type_info& obj_type() const noexcept override {
                return typeid(Tobj);
            }

            std::unique_ptr<IAnon> copy() const override {
                return std::unique_ptr<IAnon>( new AnonImpl<Ty>(obj_) );
            }
        };

        template<typename Ty>
        inline void ensure_compat_type() const noexcept(false) {
            if (!is<Ty>()) {
                throw invalid_cast_exception(
                    "Invalid cast to '" + std::string(typeid(Ty).name()) + "' underlying object is '" + anon_->obj_type().name() + "'"
                );
            }
        }

        template<typename Ty>
        struct anon_cast {
            using type_ret = Ty*;

            static inline type_ret that(const anon_ptr &ptr) noexcept(false) {
                ptr.ensure_compat_type<Ty>();
                return static_cast<Ty *>(ptr.anon_->obj_addr());
            }
        };

        template<typename Ty>
        struct anon_cast<Ty *> {
            using type_ret = Ty*;

            static inline type_ret that(const anon_ptr &ptr) noexcept(false) {
                ptr.ensure_compat_type<Ty>();
                return static_cast<Ty *>(ptr.anon_->obj_addr());
            }
        };

        template<typename Ty>
        struct anon_cast<Ty&> {
            using type_ret = Ty&;

            static inline type_ret that(const anon_ptr &ptr) noexcept(false) {
                ptr.ensure_compat_type<Ty>();
                return *static_cast<Ty *>(ptr.anon_->obj_addr());
            }
        };

        template<typename Ty>
        struct anon_cast<Ty*&> {
            using type_ret = Ty*&;

            static inline type_ret that(const anon_ptr &ptr) noexcept(false) {
                static_assert(false, "Reference to lvalue pointer is not allowed");
            }
        };

        template<typename Ty>
        struct anon_cast<Ty&&> {
            using type_ret = Ty&&;

            static inline type_ret that(const anon_ptr &ptr) noexcept(false) {
                ptr.ensure_compat_type<Ty>();
                return *static_cast<Ty *>(ptr.anon_->obj_addr());
            }
        };

        std::unique_ptr<IAnon> anon_;

        explicit anon_ptr(IAnon *anon):
            anon_(anon)
        { }

    public:
        class invalid_cast_exception : public std::bad_cast {
        private:
            std::string err_;

        public:
            invalid_cast_exception(std::string err):
                err_( std::move(err) )
            { }

            const char* what() const noexcept override {
                return err_.c_str();
            }
        };

        anon_ptr(anon_ptr &&other) noexcept:
            anon_( std::move(other.anon_) )
        { }

        anon_ptr(const anon_ptr &other) noexcept:
            anon_( other.anon_->copy() )
        { }

        template<typename Ty, typename = typename std::enable_if<
            !std::is_same<anon_ptr, typename value_of<Ty>::type>::value
        >::type>
        anon_ptr(Ty&& obj):
            anon_( new AnonImpl<Ty>( std::forward<Ty>(obj) ) )
        { }

        anon_ptr& operator =(anon_ptr &&other) noexcept = default;
        anon_ptr& operator =(const anon_ptr &other) = default;

        template<typename Ty>
        inline typename anon_cast<Ty>::type_ret get() const {
            return anon_cast<Ty>::that(*this);
        }

        inline const std::type_info& type() const {
            return anon_->obj_type();
        }

        template<typename Ty>
        inline bool is() const noexcept {
            using Tobj = typename value_of<Ty>::type;

            return typeid(Tobj) == anon_->obj_type();
        }

        template<typename Ty, typename ...Args>
        static anon_ptr make(Args&& ...args) {
            return anon_ptr(static_cast<IAnon *>(
                new AnonImpl<Ty>( std::forward<Args>(args) ... )
            ));
        }
    };
}
