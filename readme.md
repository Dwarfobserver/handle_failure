
# Handle failure

This header-only C++17 tiny library is an attempt to solve some problems in error handling :
It allows to efficiently handle irrecoverable errors with a terse syntax, and without intermediate result.
It comes with a throwing implementation (used with the 'unwrap' syntax below).

Example (using the given implementation) :

```cpp

#include <handle_failure.hpp>
#include <iostream>

// Some functions that can fail.
std::pair<std::string, std::error_code> make_string();
std::optional<int> to_int(std::string_view str);
std::error_code consume(int);

int main() {
    try {
        // The value is extracted fom the return type.
        const auto str = make_string() >> hf::unwrap();

        // A macro is available to pass context informations in case of failure.
        const auto val = to_int(str) >> HF_UNWRAP("While parsing '", str, '\'');

        // Equialent without macro : The context informations are in a lambda to
        // only evaluate them on failure.
        consume(val) >> hf::unwrap([&] (auto&& formatter) {
            formatter("While consuming ", std::to_string(val));
        });
    }
    catch (hf::unwrap_error const& e) {
        // The exception describes the error and add the given context informations.
        std::cout << e.what() << '\n';
    }
}
```

The library is based on two elements, 'hf::error_traits<T>' and 'hf::handle_failure(f, args...)'.
They are described at the beginning of 'handle_failure.hpp' or 'handle_failure/core.hpp'.
