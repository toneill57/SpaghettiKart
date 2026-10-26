#pragma once
// FNV-1a 64-bit + the field-wise state-hash vocabulary (FASE 1.3 introduced the byte hasher for the archive/binary hashes;
// FASE 1.4 adds canonical little-endian scalar writers, named fields, the FieldSink visitor and the group table).
// Header-only and std-only: compiled into the game and into oracle_tests. No game header may be included here.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace oracle {

constexpr uint64_t kFnvOffset = 0xcbf29ce484222325ull;
constexpr uint64_t kFnvPrime = 0x100000001b3ull;

// Receives every hashed field with its dotted path ("players.gPlayers[3].pos[1]"), a type tag and the raw bits
// (FASE 1.6's --oracle-dump-at). The hash itself never depends on whether a sink is attached.
struct FieldSink {
    virtual ~FieldSink() = default;
    virtual void field(const std::string& path, const char* type, uint64_t bits) = 0;
};

class Hasher {
  public:
    explicit Hasher(FieldSink* sink = nullptr) : sink_(sink) {
    }

    void bytes(const void* data, size_t n) {
        const auto* p = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < n; i++) {
            h_ ^= p[i];
            h_ *= kFnvPrime;
        }
        count_ += n;
    }
    uint64_t value() const {
        return h_;
    }
    uint64_t byteCount() const {
        return count_;
    }

    // Canonical little-endian serialization, independent of the host's endianness. Named Write*, not U32() & co.:
    // libultraship defines a function-like macro U32(x) (libultra/r4300.h:5) that would rewrite every such call.
    void WriteU8(uint8_t v) {
        bytes(&v, 1);
    }
    void WriteU16(uint16_t v) {
        const uint8_t b[2] = { uint8_t(v), uint8_t(v >> 8) };
        bytes(b, 2);
    }
    void WriteU32(uint32_t v) {
        uint8_t b[4];
        for (int i = 0; i < 4; i++) {
            b[i] = uint8_t(v >> (8 * i));
        }
        bytes(b, 4);
    }
    void WriteU64(uint64_t v) {
        uint8_t b[8];
        for (int i = 0; i < 8; i++) {
            b[i] = uint8_t(v >> (8 * i));
        }
        bytes(b, 8);
    }
    void WriteS8(int8_t v) {
        WriteU8(uint8_t(v));
    }
    void WriteS16(int16_t v) {
        WriteU16(uint16_t(v));
    }
    void WriteS32(int32_t v) {
        WriteU32(uint32_t(v));
    }
    void WriteF32(float v) { // the bit pattern: -0.0f != 0.0f, a NaN payload is kept
        uint32_t u;
        std::memcpy(&u, &v, 4);
        WriteU32(u);
    }
    void WriteF64(double v) {
        uint64_t u;
        std::memcpy(&u, &v, 8);
        WriteU64(u);
    }

    // Named writers: hash exactly like the unnamed ones, and report the field to the sink (if any).
    void WriteU8(const char* n, uint8_t v) {
        emit(n, "u8", v);
        WriteU8(v);
    }
    void WriteU16(const char* n, uint16_t v) {
        emit(n, "u16", v);
        WriteU16(v);
    }
    void WriteU32(const char* n, uint32_t v) {
        emit(n, "u32", v);
        WriteU32(v);
    }
    void WriteU64(const char* n, uint64_t v) {
        emit(n, "u64", v);
        WriteU64(v);
    }
    void WriteS8(const char* n, int8_t v) {
        emit(n, "s8", uint8_t(v));
        WriteS8(v);
    }
    void WriteS16(const char* n, int16_t v) {
        emit(n, "s16", uint16_t(v));
        WriteS16(v);
    }
    void WriteS32(const char* n, int32_t v) {
        emit(n, "s32", uint32_t(v));
        WriteS32(v);
    }
    void WriteF32(const char* n, float v) {
        uint32_t u;
        std::memcpy(&u, &v, 4);
        emit(n, "f32", u);
        WriteU32(u);
    }
    void WriteF64(const char* n, double v) {
        uint64_t u;
        std::memcpy(&u, &v, 8);
        emit(n, "f64", u);
        WriteU64(u);
    }

    // RAII path segment: Scope a(h, "players"); Scope b(h, 3); → "players[3]". Paths are built only when a sink is attached.
    class Scope {
      public:
        Scope(Hasher& h, const char* name) : h_(h), len_(h.path_.size()) {
            if (h.sink_ != nullptr && *name != '\0') {
                if (!h.path_.empty()) {
                    h.path_ += '.';
                }
                h.path_ += name;
            }
        }
        Scope(Hasher& h, size_t index) : h_(h), len_(h.path_.size()) {
            if (h.sink_ != nullptr) {
                h.path_ += "[" + std::to_string(index) + "]";
            }
        }
        ~Scope() {
            h_.path_.resize(len_);
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

      private:
        Hasher& h_;
        size_t len_;
    };

  private:
    void emit(const char* n, const char* type, uint64_t bits) {
        if (sink_ == nullptr) {
            return;
        }
        std::string p = path_;
        if (*n != '\0') {
            if (!p.empty()) {
                p += '.';
            }
            p += n;
        }
        sink_->field(p, type, bits);
    }

    uint64_t h_ = kFnvOffset;
    uint64_t count_ = 0;
    FieldSink* sink_;
    std::string path_;
};

// Scalars. Game structs get an explicit field-wise overload (OracleStatePut.h, game target only); anything else fails to
// compile here rather than being hashed as raw bytes.
template <typename T> void put(Hasher& h, const char* n, const T& v) {
    static_assert(!std::is_pointer_v<T>, "oracle: never hash a pointer (process-local address)");
    if constexpr (std::is_enum_v<T>) {
        put(h, n, static_cast<std::underlying_type_t<T>>(v));
    } else if constexpr (std::is_same_v<T, bool>) {
        h.WriteU8(n, v ? 1 : 0);
    } else if constexpr (std::is_same_v<T, float>) {
        h.WriteF32(n, v);
    } else if constexpr (std::is_same_v<T, double>) {
        h.WriteF64(n, v);
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 1) {
        if constexpr (std::is_signed_v<T>) {
            h.WriteS8(n, int8_t(v));
        } else {
            h.WriteU8(n, uint8_t(v));
        }
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 2) {
        if constexpr (std::is_signed_v<T>) {
            h.WriteS16(n, int16_t(v));
        } else {
            h.WriteU16(n, uint16_t(v));
        }
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        if constexpr (std::is_signed_v<T>) {
            h.WriteS32(n, int32_t(v));
        } else {
            h.WriteU32(n, uint32_t(v));
        }
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 8) {
        h.WriteU64(n, uint64_t(v)); // size_t / long: canonically 8 bytes, whatever the exact type
    } else {
        static_assert(sizeof(T) == 0, "oracle::put: add a field-wise overload in OracleStatePut.h");
    }
}

// Arrays, including nested ones (Mat3 = f32[3][3]): "name[i][j]".
template <typename T, size_t N> void put(Hasher& h, const char* n, const T (&a)[N]) {
    Hasher::Scope s(h, n);
    for (size_t i = 0; i < N; i++) {
        Hasher::Scope e(h, i);
        put(h, "", a[i]);
    }
}

enum Group : int { kRng, kRace, kPlayers, kControllers, kObjects, kActors, kOObjects, kCameras, kRaceArrays, kGroupCount };
inline constexpr const char* kGroupNames[kGroupCount] = { "rng",     "race",   "players",  "controllers", "objects",
                                                          "actors",  "oobjects", "cameras", "racearrays" };

struct GroupHashes {
    uint64_t g[kGroupCount];
    uint64_t all;
};

// `all` = FNV-1a-64 over the nine group values as little-endian u64, in group order.
inline uint64_t Combine(const uint64_t (&g)[kGroupCount]) {
    Hasher h;
    for (uint64_t v : g) {
        h.WriteU64(v);
    }
    return h.value();
}

// Bit i set ⇔ group i differs (the straddle probe, FASE 1.4.5). `all` is not a group of its own here.
inline uint32_t DifferingGroups(const GroupHashes& a, const GroupHashes& b) {
    uint32_t mask = 0;
    for (int i = 0; i < kGroupCount; i++) {
        if (a.g[i] != b.g[i]) {
            mask |= 1u << i;
        }
    }
    return mask;
}

} // namespace oracle
