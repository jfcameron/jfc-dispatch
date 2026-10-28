// © Joseph Cameron - MIT License

#ifndef JFC_DISPATCH_H
#define JFC_DISPATCH_H

#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace jfc::dispatch {
    using chunk_type = std::function<void(std::size_t aChunkIndex)>;

    /// \brief runs every chunk, on any threads, and returns only once all have finished; it must neither return nor
    /// throw before then, nor drop a chunk
    using task_dispatcher_type = std::function<void(std::size_t aChunkCount, const chunk_type &aChunk)>;

    using task_type = std::function<void()>;

    using task_submitter_type = std::function<void(std::vector<task_type> &&aTasks)>;

    /// \brief every chunk runs, with or without a dispatcher
    /// \throws the first exception a chunk threw, once every chunk has finished
    inline void run(const task_dispatcher_type &aDispatcher, const std::size_t aCount, const chunk_type &aChunk) {
        if (!aCount) return;

        std::exception_ptr failure;

        if (!aDispatcher || aCount == 1) {
            for (std::size_t i = 0; i < aCount; ++i) {
                try {
                    aChunk(i);
                }
                catch (...) {
                    if (!failure) failure = std::current_exception();
                }
            }
        }
        else {
            std::mutex failing;

            aDispatcher(aCount, [&](const std::size_t aIndex) {
                try {
                    aChunk(aIndex);
                }
                catch (...) {
                    const std::lock_guard lock(failing);

                    if (!failure) failure = std::current_exception();
                }
            });
        }

        if (failure) std::rethrow_exception(failure);
    }

    /// \brief every task runs, here and in order
    /// \throws the first exception a task threw, once every task has run
    inline void run_inline(std::vector<task_type> &&aTasks) {
        std::exception_ptr failure;

        for (auto &each : aTasks) {
            try {
                each();
            }
            catch (...) {
                if (!failure) failure = std::current_exception();
            }
        }

        if (failure) std::rethrow_exception(failure);
    }

    template <typename pool_type>
    [[nodiscard]] task_dispatcher_type dispatcher_of(pool_type &aPool) {
        return [&aPool](const std::size_t aCount, const chunk_type &aChunk) { aPool.run_and_wait(aCount, aChunk); };
    }

    template <typename pool_type>
    [[nodiscard]] task_submitter_type submitter_of(pool_type &aPool) {
        return [&aPool](std::vector<task_type> &&aTasks) { aPool.add_tasks(std::move(aTasks)); };
    }
}

#endif
