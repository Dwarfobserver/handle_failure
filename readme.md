
# Handle failure

This header-only C++17 tiny library is an attempt to solve some problems in error handling :
It allows to efficiently handle irrecoverable errors with a terse syntax, and without intermediate result.

It provides the core functionnalities in 'handle_failure/core.hpp' and a basic implementation in the two other files 'handle_failure/error_traits.hpp' and 'handle_failure/unwrap.hpp'. 'handle_failure.hpp' is an amalgation of these three tiles.

Example (using the given throwing implementation) :

```cpp

#include <handle_failure.hpp>

// Some functions that can fail.
std::pair<std::string, std::error_code> make_string();
std::optional<int> to_int(std::string_view str);
std::error_code consume(int);

int main() {
    try {
        // The value is extracted fom the return type.
        const auto str = make_string() >> hf::unwrap();

        // A macro is available to pass context informations.
        const auto val = to_int(argv[1]) >> HF_UNWRAP("While parsing '", str, '\'');

        // Equialent without macro : The context informations are in a lambda to
        // only evaluate them on failure.
        consume(val) >> hf::unwrap([&] (auto&& formatter) {
            formatter("While consuming ", std::to_string(val));
        });
    }
    catch (hf::unwrap_error const& e) {
        // The exception will describe the error and add the given context informations.
        std::cout << e.what() << '\n';
    }
}
```

The library is based on two elements, 'hf::error_traits<T>' and 'hf::handle_failure(f, args...)'.
They are described at the beginning of 'handle_failure.hpp' or 'handle_failure/core.hpp'.
