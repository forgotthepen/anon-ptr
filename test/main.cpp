#include "anon-ptr/anon-ptr.hpp"
#include <iostream>
#include <exception>


static bool ctor_called = false;
static bool dtor_called = false;

static int identity = 0;

struct MyTypeBase {
    MyTypeBase() {
        std::cout << "MyTypeBase ctor()" << '\n';
    }

    virtual ~MyTypeBase() {
        std::cout << "MyTypeBase dtor()" << '\n';
    }
};

struct MyType : MyTypeBase {
    int iden;

    MyType():
        iden(++identity)
    {
        ctor_called = true;
        dtor_called = false;
        std::cout << "MyType ctor(), identity=" << iden << '\n';
    }

    MyType(MyType &&other):
        MyType()
    {
        std::cout << "MyType move ctor()" << '\n';
    }

    MyType(const MyType &other):
        MyType()
    {
        std::cout << "MyType copy ctor()" << '\n';
    }

    ~MyType() {
        ctor_called = false;
        dtor_called = true;
        std::cout << "MyType dtor(), identity=" << iden << '\n';
        iden = 0;
    }
};

int main() {
    std::cout << "hello" << '\n';

    const auto float_ptr = nonstd::anon_ptr::make_anon<float>(3.7f);
    std::cout << "anon type [float_ptr]: " << float_ptr.type().name() << '\n';

    try {
        nonstd::anon_ptr any_ptr = 33.654;
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';
    
        any_ptr = 27;
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';

        any_ptr = MyType{};
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';

        {
            nonstd::anon_ptr any_ptr_copy = any_ptr;
            std::cout << "  anon type [any_ptr_copy]: " << any_ptr.type().name() << '\n';
        }

        any_ptr = "some string";
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    try {
        std::cout << float_ptr.get<float>() << '\n';
        std::cout << float_ptr.get<float*>() << '\n';
        std::cout << float_ptr.get<float&>() << '\n';
        std::cout << float_ptr.get<const float>() << '\n';
        std::cout << float_ptr.get<const float*>() << '\n';
        std::cout << float_ptr.get<const float&>() << '\n';
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    {
        const auto my_type_ptr = nonstd::anon_ptr::make_anon<MyType>();

        if (!ctor_called) {
            std::cerr << "Error: MyType ctor was not called" << '\n';
            return 1;
        }
    
        try {
            my_type_ptr.get<MyType>();
        } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
            std::cerr << "Error: " << ex.what() << '\n';
            return 1;
        }
    
        // must get exact type
        // TODO is this not desirable?
        try {
            my_type_ptr.get<MyTypeBase>();
            return 1;
        } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
            std::cerr << "Error: " << ex.what() << '\n';
        }
    }

    if (!dtor_called) {
        std::cerr << "Error: MyType dtor was not called" << '\n';
        return 1;
    }

    try {
        std::cout << float_ptr.get<int>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    try {
        std::cout << float_ptr.get<int*>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    try {
        std::cout << float_ptr.get<int&>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    try {
        std::cout << float_ptr.get<const int>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    try {
        std::cout << float_ptr.get<const int *>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    try {
        std::cout << float_ptr.get<const int&>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    std::cout << "\n\nbye!!" << '\n';
    return 0;
}
