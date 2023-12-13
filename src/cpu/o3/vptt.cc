#include "cpu/o3/lvpt.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/LVPUnit.hh"

namespace gem5
{

namespace o3
{

LVPT::LVPT(unsigned _numEntries, unsigned _shiftAmt, unsigned _numThreads)
    : numEntries(_numEntries),
      shiftAmt(_shiftAmt),
      log2NumThreads(floorLog2(_numThreads))
{
    DPRINTF(LVPUnit, "LVPT: Creating LVPT object.\n");

    if (!isPowerOf2(numEntries)) {
        fatal("LVPT entries is not a power of 2!");
    }

    lvpt.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }

    idxMask = numEntries - 1;
}

void LVPT::reset()
{
    DPRINTF(LVPUnit, "LVPT reset.\n");
    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }
}

inline unsigned LVPT::getIndex(Addr loadAddr, ThreadID tid)
{
    // Need to shift load address over by the word offset.
    return (loadAddr >> shiftAmt) & idxMask;
}

bool LVPT::valid(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid) {
        return true;
    } else {
        return false;
    }
}

uint64_t LVPT::lookup(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    // DPRINTF(LVPUnit, "LVPT: Looking up 0x%x (idx %u) for tid %u\n", loadAddr, lvpt_idx, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid ) {
        uint64_t value = lvpt[lvpt_idx].value;
        // DPRINTF(LVPUnit, "found pred val 0x%x\n", value);
        return value;
    } else {
        // DPRINTF(LVPUnit, "no valid pred found\n");
        return 0xDD;
    }
}

void LVPT::update(Addr loadAddr, uint64_t loadValue, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    // DPRINTF(LVPUnit, "LVPT: Updating 0x%x (idx %u) for tid %u with %llu\n", loadAddr, lvpt_idx, tid, loadValue);

    assert(lvpt_idx < numEntries);

    lvpt[lvpt_idx].tid = tid;
    lvpt[lvpt_idx].valid = true;
    lvpt[lvpt_idx].value = loadValue;
}

} // namespace o3
} // namespace gem5
