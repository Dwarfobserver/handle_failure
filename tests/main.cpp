
#include <handle_failure.hpp>
#include <iostream>

namespace {
    const auto fail_errc = std::error_code{ 1, std::system_category() };

    auto case_count = 0;

    template <class F>
    bool returns_error(F&& f) {
        ++case_count;
        try {
            f() >> HF_UNWRAP("Failure in case ", case_count);
            return false;
        }
        catch (hf::unwrap_error const&) {
            return true;
        }
    }
} // ::<anon>

int main() {
    return  returns_error([] () -> std::optional<int>                    { return 1; })
        || !returns_error([] () -> std::optional<int>                    { return {}; }) 
        ||  returns_error([] () -> std::pair<int, std::error_code>       { return {}; })
        || !returns_error([] () -> std::pair<int, std::error_code>       { return { 0, fail_errc }; })
        ||  returns_error([] () -> std::tuple<int, std::error_code>      { return {}; })
        || !returns_error([] () -> std::tuple<int, std::error_code>      { return { 0, fail_errc }; })
        ||  returns_error([] () -> std::tuple<int, int, std::error_code> { return {}; })
        || !returns_error([] () -> std::tuple<int, int, std::error_code> { return { 0, 0, fail_errc }; });
}
