#pragma once

namespace sdk {
template <typename T> struct NativeArray {
    T* begin() const {
        if (elements == nullptr || num == 0 || num_allocated == 0) {
            return nullptr;
        }

        return elements;
    }

    T* end() const {
        if (elements == nullptr || num == 0 || num_allocated == 0) {
            return nullptr;
        }

        return elements + num;
    }

    T* elements;
    uint32_t num;
    uint32_t num_allocated;
};
} // namespace sdk