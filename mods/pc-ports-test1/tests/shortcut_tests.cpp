#include "../common/shortcut.hpp"

extern "C" uint64_t shvnx_shortcut_tests() {
    uint32_t passed = 0, failed = 0;
    auto check = [&](bool ok) {
        if (ok)
            ++passed;
        else
            ++failed;
    };
    for (unsigned i = 0; i < 10; ++i) {
        check(Port::MenuButtons[i] != 187 && Port::MenuButtons[i] != 203);
        for (unsigned j = 0; j < i; ++j)
            check(Port::MenuButtons[i] != Port::MenuButtons[j]);
    }
    for (unsigned order = 0; order < 2; ++order) {
        Port::Shortcut shortcut;
        check(!shortcut.Poll(false, false));
        check(!shortcut.Poll(order == 0, order != 0));
        check(shortcut.Poll(true, true));
        check(!shortcut.NavigationReady(true, true));
        for (int frame = 0; frame < 60; ++frame)
            check(!shortcut.Poll(true, true));
        check(!shortcut.NavigationReady(true, false));
        check(!shortcut.NavigationReady(false, true));
        check(shortcut.NavigationReady(false, false));
        check(!shortcut.Poll(false, false));
        check(shortcut.Poll(true, true));
        shortcut.Cancel(true, true);
        check(!shortcut.Poll(true, true));
        check(!shortcut.Poll(false, false));
        check(shortcut.NavigationReady(false, false));
        check(shortcut.Poll(true, true));
    }
    return (uint64_t(passed) << 32) | failed;
}
#ifndef SHVNX_FREESTANDING_TEST
int main() { return uint32_t(shvnx_shortcut_tests()) ? 1 : 0; }
#endif
