# Anonymous pointer to any element type for homogeneous collections

The type of the containing/pointer class itself `anon_ptr` is non-templated, but the underlying pointer can point to an object of any type.  
After object creation, the underlying object is type erased, and some runtime type information is kept for later usage.  
This makes it possible to create a homogeneous collection (example: `std::vector<anon_ptr>`) of objects whose class/type is different/unrelated.  

## Features
* Single header library
* Compatible with `C++11`
* Arguments forwarding to the constructor of the underlying type (via `anon_ptr::make<...>(...)`), making it very flexible to support custom types
* Runtime-safe getter (`.get<Type>()`), throws the exception (`anon_ptr::invalid_cast_exception`) if the requested type is not the same as the original creation type

## Object ownership
When assigning a non-pointer instance to `anon_ptr` the underlying object is either allocated, copied, or moved, depending on the context, and the ownership is transferred to `anon_ptr`.  
But when assigning a pointer to `anon_ptr` it acts as just a wrapper, ownership of the underlying object is not transferred.  

## Example (point at anything):
```c++
#include "anon-ptr/anon-ptr.hpp"
#include <iostream>
#include <exception>

int main() {
    std::cout << "hello" << '\n';

    try {
        nonstd::anon_ptr any_ptr = 33.654;
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';
        // MSVC: anon type [any_ptr]: double
    
        any_ptr = 27;
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';
        // MSVC: anon type [any_ptr]: int

        any_ptr = "some string";
        std::cout << "anon type [any_ptr]: " << any_ptr.type().name() << '\n';
        // MSVC: anon type [any_ptr]: char const * __ptr64
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "\n\nbye!!" << '\n';
    return 0;
}
```

## Example (point at another pointer/array):
```c++
#include "anon-ptr/anon-ptr.hpp"
#include <iostream>
#include <exception>

int main() {
    std::cout << "hello" << '\n';

    try {
        char arr[] = { 5, 6, 7};

        nonstd::anon_ptr arr_ptr = arr;
        std::cout << "anon type [arr_ptr]: " << arr_ptr.type().name() << '\n';
        arr_ptr.get<char *>();

        // arr is not deallocated after this scope
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "\n\nbye!!" << '\n';
    return 0;
}
```

## Example (runtime-safe getter):
```c++
#include "anon-ptr/anon-ptr.hpp"
#include <iostream>
#include <exception>
// #include <typeinfo> // if you want to use std::bad_cast

int main() {
    std::cout << "hello" << '\n';

    const auto float_ptr = nonstd::anon_ptr::make<float>(3.7f);
    std::cout << "anon type [float_ptr]: " << float_ptr.type().name() << '\n';
    // MSVC: anon type [float_ptr]: float

    // valid call to get()
    try {
        std::cout << float_ptr.get<float>() << '\n'; // returns float *
        std::cout << float_ptr.get<const float>() << '\n'; // returns float const *

        std::cout << float_ptr.get<float &>() << '\n'; // returns ref to underlying object
        std::cout << float_ptr.get<const float&>() << '\n'; // returns const ref to underlying object
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    // invalid calls to get()
    try {
        std::cout << float_ptr.get<int>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) { // can also use std::bad_cast
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    try {
        std::cout << float_ptr.get<int *>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    try {
        std::cout << float_ptr.get<int &>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    try {
        std::cout << float_ptr.get<const int>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    try {
        std::cout << float_ptr.get<const int *>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    try {
        std::cout << float_ptr.get<const int &>() << '\n';
        return 1;
    } catch (const nonstd::anon_ptr::invalid_cast_exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        // MSVC: Error: Invalid cast to 'int' underlying object is 'float'
    }

    std::cout << "\n\nbye!!" << '\n';
    return 0;
}
```

## Example (check runtime type):
```c++
#include "anon-ptr/anon-ptr.hpp"
#include <iostream>
#include <exception>
// #include <typeinfo> // if you want to use std::bad_cast

int main() {
    std::cout << "hello" << '\n';

    const auto float_ptr = nonstd::anon_ptr::make<float>(3.7f);
    if (!float_ptr.is<float>()) {
        std::cerr << "Error: not a float" << '\n';
        return 1;
    } else {
        std::cout << "Float type" << '\n';
    }

    std::cout << "\n\nbye!!" << '\n';
    return 0;
}
```
