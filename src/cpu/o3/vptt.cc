#include "cpu/o3/vptt.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/LVPUnit.hh"

namespace gem5
{

namespace o3
{

VPTT::VPTT(unsigned _numEntries, unsigned _shiftAmt, unsigned _numThreads)
    : numEntries(_numEntries),
      shiftAmt(_shiftAmt),
      log2NumThreads(floorLog2(_numThreads))
{
    DPRINTF(LVPUnit, "VPTT: Creating VPTT object.\n");

    if (!isPowerOf2(numEntries)) {
        fatal("VPTT entries is not a power of 2!");
    }

    vptt.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        vptt[i].valid = false;
    }

    idxMask = numEntries - 1;
}

void VPTT::reset()
{
    DPRINTF(LVPUnit, "VPTT reset.\n");
    for (unsigned i = 0; i < numEntries; ++i) {
        vptt[i].valid = false;
    }
}

bool VPTT::lookup(InstSeqNum seq_num, ThreadID tid, VPTTEntry& entry)
{
    // There might be a better way to do this, but I don't have time
    bool found = false;
    for (unsigned i = 0; i < numEntries; ++i) {
        if (vptt[i].valid && vptt[i].seq_num == seq_num && vptt[i].tid == tid) {
            entry = vptt[i];
            found = true;
            break;
        }
    }

    return found;
}

void VPTT::insert(InstSeqNum seq_num, Addr loadAddr, ThreadID tid) {
    // Just find the first empty space to put it
    bool found = false;
    for (unsigned i = 0; i < numEntries; ++i) {
        if (!vptt[i].valid) {
            vptt[i] = {
                .seq_num = seq_num,
                .addr = loadAddr,
                .tid = tid,
                .valid = true,
            };
            found = true;
            break;
        }
    }

    if (!found) {
        panic("No space left in VPTT");
    }
}

void VPTT::remove(InstSeqNum seq_num, ThreadID tid) {
    // There might be a better way to do this, but I don't have time
    bool found = false;
    for (unsigned i = 0; i < numEntries; ++i) {
        if (vptt[i].valid && vptt[i].seq_num == seq_num && vptt[i].tid == tid) {
            vptt[i].valid = false;
            found = true;
            break;
        }
    }
}

} // namespace o3
} // namespace gem5
