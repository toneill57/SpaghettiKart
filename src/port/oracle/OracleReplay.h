#pragma once
// Replay decisions (FASE 1.5): header compatibility, the iteration cursor, pad application and the hash-log path guard.
// Std-only: compiled into the game and into oracle_tests. No game or libultraship header may be included here.
#include "OracleRecording.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oracle {

struct Compat {
    bool ok = true;
    bool archivesUnchecked = false; // both archive hashes 0 in recording and live: equal zeros prove nothing
    std::string field, recorded, live; // the first refused field, in the user-visible order
    std::vector<std::string> notes;    // informational differences (buildVersion, binaryHash), never refused
};

// Refuses on the simulation fields only (CONTEXTO "Build identity vs replay refusal"). The seed is NOT compared (1.7 writes
// gRandomSeed16 = header.seed); magic / version / headerBytes were already checked by read().
Compat CheckCompatible(const Header& rec, const Header& live);

enum class Step { Feed, End, Misaligned };

class ReplayCursor {
  public:
    explicit ReplayCursor(const Recording& r) : r_(r) {
    }
    // Feed: *out = the record for this iteration, the cursor advances. End: no records left. Misaligned: *recIter = the
    // iteration the file holds next (the cursor does not advance past it).
    Step next(uint32_t iteration, const PadsRecord** out, uint32_t* recIter);
    size_t fed() const {
        return i_;
    }

  private:
    const Recording& r_;
    size_t i_ = 0;
};

// PadT = OSContPad in the game. Writes exactly the five fields game code reads (src/main.c update_controller); err_no,
// gyro_x and gyro_y are untouched by design. Overwrites, never ORs: a live controller has no effect on these fields.
template <class PadT> void ApplyRecordedPads(const PadsRecord& r, PadT* pads) {
    for (int i = 0; i < 4; i++) {
        pads[i].button = r.pads[i].button;
        pads[i].stick_x = r.pads[i].stick_x;
        pads[i].stick_y = r.pads[i].stick_y;
        pads[i].right_stick_x = r.pads[i].right_stick_x;
        pads[i].right_stick_y = r.pads[i].right_stick_y;
    }
}

// "" when the replay may write its hash log at hashLogPath, else the reason to refuse (exit 2). Refused: the DEFAULTED path
// (<replay>.mk64hash, also the recording run's default) when it already exists, and any path that is the recording itself.
// An explicit --oracle-hash-log otherwise always wins.
std::string CheckReplayHashLogPath(const std::string& replayPath, const std::string& hashLogPath, bool hashLogDefaulted);

} // namespace oracle
