// © Joseph Cameron - MIT License

#include <jfc/dispatch.h>

#include <cstddef>
#include <vector>

int main() {
    std::vector<std::size_t> squares(4);

    jfc::dispatch::run({}, squares.size(), [&](const std::size_t &aIndex) { squares[aIndex] = aIndex * aIndex; });

    int ran = 0;

    std::vector<jfc::dispatch::task_type> tasks;
    tasks.emplace_back([&] { ++ran; });

    jfc::dispatch::run_inline(std::move(tasks));

    return squares == std::vector<std::size_t>{0, 1, 4, 9} && ran == 1 ? 0 : 1;
}
