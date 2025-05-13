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

#include <utility> // std::forward, std::move
#include <typeinfo> // std::type_info, std::bad_cast
#include <type_traits> // std::decay, std::enable_if, std::is_same, std::add_pointer
#include <string>


namespace nonstd {
    class anon_ptr {
    private:
        template<typename Ty>
        using value_of = std::decay<Ty>;

        template<typename Ty>
        using no_cvref = std::remove_cv< typename std::remove_reference<Ty>::type >;

        struct IAnon {
            virtual ~IAnon() = default;

            virtual void* obj_addr() noexcept = 0;
            virtual const std::type_info& obj_type() const noexcept = 0;
            virtual void copy(void *mem) const = 0;
            virtual void move(void *mem) noexcept = 0;
        };

        template<typename Ty>
        class AnonImpl : public IAnon {
        private:
            using Tobj = typename value_of<Ty>::type;
            using TobjPtr = typename std::add_pointer<Tobj>::type;

            struct disambig {};

            TobjPtr obj_ptr_;

            AnonImpl(disambig, TobjPtr ptr):
                obj_ptr_(ptr)
            { }

        public:
            template<typename ...Args>
            AnonImpl(Args&& ...args):
                obj_ptr_(new Tobj( std::forward<Args>(args) ... ))
            { }

            ~AnonImpl() override {
                if (obj_ptr_ != nullptr) {
                    delete obj_ptr_;
                    obj_ptr_ = nullptr;
                }
            }

            void* obj_addr() noexcept override {
                return reinterpret_cast<void *>(obj_ptr_);
            }

            const std::type_info& obj_type() const noexcept override {
                return typeid(Tobj);
            }

            void copy(void *mem) const override {
                new (mem) AnonImpl<Ty>( *obj_ptr_ );
            }

            void move(void *mem) noexcept override {
                new (mem) AnonImpl<Ty>( disambig{}, obj_ptr_ );
                obj_ptr_ = nullptr;
            }
        };

        template<typename Ty>
        inline void ensure_compat_type() const noexcept(false) {
            if (!is<Ty>()) {
                throw invalid_cast_exception(
                    "Invalid cast to '" + std::string(typeid(Ty).name()) +
                    "' underlying object is '" + anon_->obj_type().name() + "'"
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

        template<typename>
        struct disambig {};

        template<typename Ty, typename ...Args>
        anon_ptr(disambig<Ty>, Args&& ...args):
            anon_(new (reinterpret_cast<void *>(anon_mem_)) AnonImpl<Ty>( std::forward<Args>(args) ... ))
        {
            static_assert(sizeof(AnonImpl<Ty>) <= sizeof(anon_mem_), "In-class memory is too small");
        }

        alignas(void *) char anon_mem_[2 * sizeof(AnonImpl<void *>)];
        IAnon *anon_;

    public:
        class invalid_cast_exception : public std::bad_cast {
        private:
            std::string err_;

        public:
            invalid_cast_exception(std::string err) noexcept:
                err_( std::move(err) )
            { }

            const char* what() const noexcept override {
                return err_.c_str();
            }
        };

        template<typename Ty, typename = typename std::enable_if<
            !std::is_same<anon_ptr, typename value_of<Ty>::type>::value
        >::type>
        anon_ptr(Ty&& obj):
            anon_ptr( disambig<Ty>{}, std::forward<Ty>(obj) )
        { }

        anon_ptr(anon_ptr &&other) noexcept:
            anon_(reinterpret_cast<IAnon *>(anon_mem_))
        {
            other.anon_->move( reinterpret_cast<void *>(anon_mem_) );
            other.anon_ = nullptr;
        }

        anon_ptr(const anon_ptr &other):
            anon_(reinterpret_cast<IAnon *>(anon_mem_))
        {
            other.anon_->copy( reinterpret_cast<void *>(anon_mem_) );
        }

        ~anon_ptr() {
            if (anon_ != nullptr) {
                anon_->~IAnon();
                anon_ = nullptr;
            }
        }

        anon_ptr& operator =(anon_ptr &&other) noexcept {
            if (&other != this) {
                this->~anon_ptr();
                new (this) anon_ptr( std::move(other) );
            }
            return *this;
        }

        anon_ptr& operator =(const anon_ptr &other) {
            if (&other != this) {
                this->~anon_ptr();
                new (this) anon_ptr( other );
            }
            return *this;
        }

        template<typename Ty>
        inline typename anon_cast<Ty>::type_ret get() const noexcept(false) {
            return anon_cast<Ty>::that(*this);
        }

        inline const std::type_info& type() const noexcept {
            return anon_->obj_type();
        }

        template<typename Ty>
        inline bool is() const noexcept {
            using Tobj = typename value_of<Ty>::type;

            return typeid(Tobj) == anon_->obj_type();
        }

        template<typename Ty, typename ...Args>
        static anon_ptr make(Args&& ...args) {
            return anon_ptr( disambig<Ty>{}, std::forward<Args>(args) ... );
        }
    };
}
