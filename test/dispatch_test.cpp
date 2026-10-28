// © Joseph Cameron - MIT License

#include <jfc/catch.hpp>

#include <jfc/dispatch.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace jfc::dispatch;

namespace {
    struct threaded final {
        std::atomic<bool> escaped = false;
        std::atomic<std::size_t> calls = 0;

        [[nodiscard]] task_dispatcher_type dispatcher() {
            return [this](const std::size_t &aCount, const chunk_type &aChunk) {
                ++calls;

                std::vector<std::thread> threads;

                for (std::size_t i = 0; i < aCount; ++i) threads.emplace_back([this, &aChunk, i] {
                    try {
                        aChunk(i);
                    }
                    catch (...) {
                        escaped = true;
                    }
                });

                for (auto &each : threads) each.join();
            };
        }
    };

    struct in_order final {
        std::size_t calls = 0;

        [[nodiscard]] task_dispatcher_type dispatcher() {
            return [this](const std::size_t &aCount, const chunk_type &aChunk) {
                ++calls;

                for (std::size_t i = 0; i < aCount; ++i) aChunk(i);
            };
        }
    };

    struct tally final {
        std::mutex mutex;
        std::vector<int> times;

        explicit tally(const std::size_t aCount) : times(aCount, 0) {}

        void ran(const std::size_t aIndex) {
            const std::lock_guard lock(mutex);

            ++times.at(aIndex);
        }
    };

    [[nodiscard]] std::vector<std::size_t> squares(const task_dispatcher_type &aDispatcher, const std::size_t aCount) {
        std::vector<std::size_t> out(aCount);

        run(aDispatcher, aCount, [&](const std::size_t &aIndex) { out[aIndex] = aIndex * aIndex; });

        return out;
    }

    template <typename do_type>
    [[nodiscard]] std::string thrown_by(do_type &&aDo) {
        try {
            aDo();
        }
        catch (const std::exception &e) {
            return e.what();
        }

        return {};
    }
}

TEST_CASE("**every chunk runs once**, with no dispatcher, one on the calling thread, and one with threads", "[dispatch]") {
    constexpr std::size_t COUNT = 16;

    threaded withThreads;
    in_order inOrder;

    for (const auto &dispatcher : {task_dispatcher_type{}, inOrder.dispatcher(), withThreads.dispatcher()}) {
        tally counted(COUNT);

        run(dispatcher, COUNT, [&](const std::size_t &aIndex) { counted.ran(aIndex); });

        REQUIRE(counted.times == std::vector<int>(COUNT, 1));
    }

    REQUIRE(inOrder.calls == 1);
    REQUIRE(withThreads.calls == 1);
}

TEST_CASE("**serial where there is no dispatcher, or only one chunk**, on the calling thread and in order", "[dispatch]") {
    in_order inOrder;

    SECTION("**one chunk is run here**, and the dispatcher is not asked") {
        std::thread::id ranOn;

        run(inOrder.dispatcher(), 1, [&](const std::size_t &) { ranOn = std::this_thread::get_id(); });

        REQUIRE(ranOn == std::this_thread::get_id());
        REQUIRE(inOrder.calls == 0);
    }

    SECTION("**none is nothing**, and the dispatcher is not asked") {
        bool ran = false;

        run(inOrder.dispatcher(), 0, [&](const std::size_t &) { ran = true; });

        REQUIRE_FALSE(ran);
        REQUIRE(inOrder.calls == 0);
    }

    SECTION("**no dispatcher**: every chunk here, in order") {
        std::vector<std::size_t> order;
        std::set<std::thread::id> threads;

        run({}, 5, [&](const std::size_t &aIndex) {
            order.push_back(aIndex);
            threads.insert(std::this_thread::get_id());
        });

        REQUIRE(order == std::vector<std::size_t>{0, 1, 2, 3, 4});
        REQUIRE(threads == std::set<std::thread::id>{std::this_thread::get_id()});
    }
}

TEST_CASE("**what a chunk throws is thrown on the calling thread**, once every chunk has finished", "[dispatch]") {
    constexpr std::size_t COUNT = 8;

    threaded withThreads;

    std::atomic<std::size_t> finished = 0;
    std::thread::id caughtOn;

    try {
        run(withThreads.dispatcher(), COUNT, [&](const std::size_t &aIndex) {
            if (aIndex == 3) throw std::runtime_error("three");

            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            ++finished;
        });

        FAIL("nothing was thrown");
    }
    catch (const std::runtime_error &e) {
        caughtOn = std::this_thread::get_id();

        REQUIRE(std::string(e.what()) == "three");
    }

    REQUIRE(caughtOn == std::this_thread::get_id());
    REQUIRE(finished == COUNT - 1);
    REQUIRE_FALSE(withThreads.escaped);
}

TEST_CASE("**where several throw, the first caught is thrown**, and the rest are dropped", "[dispatch]") {
    in_order inOrder;

    std::size_t ran = 0;

    REQUIRE(thrown_by([&] {
        run(inOrder.dispatcher(), 4, [&](const std::size_t &aIndex) {
            ++ran;

            throw std::runtime_error(std::to_string(aIndex));
        });
    }) == "0");

    REQUIRE(ran == 4);

    SECTION("**with threads, it is one of theirs**") {
        threaded withThreads;

        const auto thrown = thrown_by([&] {
            run(withThreads.dispatcher(), 4, [](const std::size_t &aIndex) {
                throw std::runtime_error(std::to_string(aIndex));
            });
        });

        REQUIRE(std::set<std::string>{"0", "1", "2", "3"}.count(thrown) == 1);
        REQUIRE_FALSE(withThreads.escaped);
    }
}

TEST_CASE("**a run with threads is the serial run**, exceptions included (code-style.md §7.1)", "[dispatch]") {
    threaded withThreads;

    REQUIRE(squares(withThreads.dispatcher(), 64) == squares({}, 64));

    const auto throwing_at = [](const task_dispatcher_type &aDispatcher, const std::size_t aThrower) {
        tally counted(32);

        const auto thrown = thrown_by([&] {
            run(aDispatcher, 32, [&](const std::size_t &aIndex) {
                counted.ran(aIndex);

                if (aIndex == aThrower) throw std::invalid_argument("thrown");
            });
        });

        return std::make_pair(thrown, counted.times);
    };

    for (const std::size_t thrower : {std::size_t{0}, std::size_t{17}, std::size_t{31}}) {
        const auto serial = throwing_at({}, thrower);

        REQUIRE(serial.first == "thrown");
        REQUIRE(serial.second == std::vector<int>(32, 1));
        REQUIRE(throwing_at(withThreads.dispatcher(), thrower) == serial);
    }

    REQUIRE_FALSE(withThreads.escaped);
}

TEST_CASE("**the serial submitter runs each task now**, in order, on the calling thread", "[dispatch]") {
    std::vector<int> order;
    std::set<std::thread::id> threads;

    std::vector<task_type> tasks;

    for (int i = 0; i < 3; ++i) tasks.emplace_back([&, i] {
        order.push_back(i);
        threads.insert(std::this_thread::get_id());
    });

    run_inline(std::move(tasks));

    REQUIRE(order == std::vector<int>{0, 1, 2});
    REQUIRE(threads == std::set<std::thread::id>{std::this_thread::get_id()});

    SECTION("**every task runs though one throws**, and the first thrown comes out after") {
        std::vector<int> ran;

        std::vector<task_type> throwing;

        for (int i = 0; i < 4; ++i) throwing.emplace_back([&, i] {
            ran.push_back(i);

            if (i == 1 || i == 2) throw std::runtime_error(std::to_string(i));
        });

        REQUIRE(thrown_by([&] { run_inline(std::move(throwing)); }) == "1");
        REQUIRE(ran == std::vector<int>{0, 1, 2, 3});
    }
}

TEST_CASE("**a dispatcher that throws** is let through, after the chunks it ran", "[dispatch]") {
    tally counted(4);

    const task_dispatcher_type refusing = [](const std::size_t aCount, const chunk_type &aChunk) {
        for (std::size_t i = 0; i < aCount; ++i) aChunk(i);

        throw std::runtime_error("the dispatcher's own");
    };

    REQUIRE(thrown_by([&] { run(refusing, 4, [&](const std::size_t &aIndex) { counted.ran(aIndex); }); })
        == "the dispatcher's own");
    REQUIRE(counted.times == std::vector<int>(4, 1));
}

TEST_CASE("**a pool's threads lent**, as a dispatcher and as a submitter", "[dispatch]") {
    struct pool final {
        std::vector<std::size_t> waitedOn;
        std::size_t added = 0;

        void run_and_wait(const std::size_t aCount, const chunk_type &aChunk) {
            waitedOn.push_back(aCount);

            for (std::size_t i = 0; i < aCount; ++i) aChunk(i);
        }

        void add_tasks(std::vector<task_type> &&aTasks) {
            added += aTasks.size();

            for (auto &each : aTasks) each();
        }
    };

    pool lent;

    SECTION("**a dispatcher**: a fork-join waits on the pool") {
        tally counted(5);

        run(dispatcher_of(lent), 5, [&](const std::size_t &aIndex) { counted.ran(aIndex); });

        REQUIRE(lent.waitedOn == std::vector<std::size_t>{5});
        REQUIRE(counted.times == std::vector<int>(5, 1));
    }

    SECTION("**a submitter**: the tasks are the pool's to run") {
        int ran = 0;

        submitter_of(lent)({[&] { ++ran; }, [&] { ++ran; }});

        REQUIRE(lent.added == 2);
        REQUIRE(ran == 2);
    }
}
