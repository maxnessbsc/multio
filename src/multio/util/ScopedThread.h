
#pragma once

#include <thread>

#include "eckit/exception/Exceptions.h"

namespace multio {
namespace util {

class ScopedThread {
public:
    explicit ScopedThread(std::thread thread) : thread_(std::move(thread)) {
        if (!thread_.joinable()) {
            throw eckit::SeriousBug("No thread");
        }
    }

    ~ScopedThread() { thread_.join(); }

    ScopedThread(const ScopedThread& rhs) = delete;
    ScopedThread& operator=(const ScopedThread& rhs) = delete;

private:  // members
    std::thread thread_;
};

}  // namespace util
}  // namespace multio
