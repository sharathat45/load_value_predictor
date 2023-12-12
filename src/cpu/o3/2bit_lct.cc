#include "cpu/o3/2bit_lct.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/LVPUnit.hh"

namespace gem5
{

namespace o3
{
         
LCT::LCT(unsigned _lctSize, unsigned _lctCtrBits, unsigned _instShiftAmt, unsigned _numThreads)
    : lctSize(_lctSize),
      lctCtrBits(_lctCtrBits),
      lctPredictorSets(lctSize),
      lctCtrs(lctPredictorSets, SatCounter8(lctCtrBits)),
      indexMask(lctPredictorSets - 1),
      instShiftAmt(_instShiftAmt)
{
    if (!isPowerOf2(lctSize)) {
        fatal("LCT: Invalid LCT size!\n");
    }

    if (!isPowerOf2(lctPredictorSets)) {
        fatal("LCT: Invalid number of LCT predictor sets! Check lctCtrBits.\n");
    }

    DPRINTF(LVPUnit, "LCT: index mask: %#x\n", indexMask);

    DPRINTF(LVPUnit, "LCT: local predictor size: %i\n", lctSize);

    DPRINTF(LVPUnit, "LCT: local counter bits: %i\n", lctCtrBits);

    DPRINTF(LVPUnit, "LCT: instruction shift amount: %i\n", instShiftAmt);
}

uint8_t LCT::lookup(ThreadID tid, Addr inst_addr)
{
    unsigned lct_idx = getLocalIndex(inst_addr);

    DPRINTF(LVPUnit, "LCT: Looking up 0x%x (idx %u) for tid %u\n", inst_addr, lct_idx, tid);

    uint8_t counter_val = lctCtrs[lct_idx];

    return counter_val;
}

bool LCT::getPrediction(uint8_t &count)
{
    // Get the MSB of the count
    return (count >> (lctCtrBits - 1));
}

void LCT::update(ThreadID tid, Addr inst_addr, bool prediction_outcome, bool squashed)
{
    unsigned lct_idx;

    // No state to restore, and we do not update on the wrong path.
    if (squashed) {
        return;
    }

    // Update the local predictor.
    lct_idx = getLocalIndex(inst_addr);

    DPRINTF(LVPUnit, "LCT: Updating 0x%x (idx %u) for tid %u with pred outcome:%d\n", inst_addr, lct_idx, tid, prediction_outcome);

    if (prediction_outcome)
    {
        lctCtrs[lct_idx]++;
        DPRINTF(LVPUnit, "LCT: PC = 0x%x ld ins address updated ++\n", inst_addr);
    }
    else
    {
        lctCtrs[lct_idx]--;
        DPRINTF(LVPUnit, "LCT: PC = 0x%x ld ins address updated --\n", inst_addr);
    }
}

inline unsigned LCT::getLocalIndex(Addr &inst_addr)
{
    return (inst_addr >> instShiftAmt) & indexMask;
}

} // namespace o3
} // namespace gem5
