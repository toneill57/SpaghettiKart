#pragma once
// FNV-1a 64-bit (FASE 1.3 needs it for the archive/binary hashes; FASE 1.4 extends this header with the field-wise state hash).
// Header-only and std-only: compiled into the game and into oracle_tests.
#include <cstddef>
#include <cstdint>

namespace oracle {

constexpr uint64_t kFnvOffset = 0xcbf29ce484222325ull;
constexpr uint64_t kFnvPrime = 0x100000001b3ull;

class Hasher {
  public:
    void bytes(const void* data, size_t n) {
        const auto* p = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < n; i++) {
            h_ ^= p[i];
            h_ *= kFnvPrime;
        }
    }
    uint64_t value() const {
        return h_;
    }

  private:
    uint64_t h_ = kFnvOffset;
};

} // namespace oracle
