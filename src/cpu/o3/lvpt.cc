#include "cpu/o3/lvpt.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/IEW.hh"

namespace gem5
{

namespace o3
{

LVPT::LVPT(unsigned _numEntries,
           unsigned _shiftAmt,
           unsigned _numThreads)
    : numEntries(_numEntries),
      shiftAmt(_shiftAmt),
      log2NumThreads(floorLog2(_numThreads))
{
    DPRINTF(IEW, "LVPT: Creating LVPT object.\n");

    if (!isPowerOf2(numEntries)) {
        fatal("LVPT entries is not a power of 2!");
    }

    lvpt.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }

    idxMask = numEntries - 1;
}

void
LVPT::reset()
{
    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }
}

inline
unsigned
LVPT::getIndex(Addr loadAddr, ThreadID tid)
{
    // Need to shift load address over by the word offset.
    return (loadAddr >> shiftAmt) & idxMask;
}

bool
LVPT::valid(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid && lvpt[lvpt_idx].tid == tid) {
        return true;
    } else {
        return false;
    }
}

RegVal
LVPT::lookup(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid && lvpt[lvpt_idx].tid == tid) {
        return lvpt[lvpt_idx].value;
    } else {
        return 0xBAD1BAD1;
    }
}

void
LVPT::update(Addr loadAddr, RegVal loadValue, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    lvpt[lvpt_idx].tid = tid;
    lvpt[lvpt_idx].valid = true;
    lvpt[lvpt_idx].value = loadValue;
}

} // namespace o3
} // namespace gem5
