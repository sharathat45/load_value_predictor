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
}

uint8_t LCT::lookup(ThreadID tid, Addr inst_addr)
{
    unsigned lct_idx = getLocalIndex(inst_addr);

    DPRINTF(LVPUnit, "LCT: Lookup PC:0x%x (idx %u)\n", inst_addr, lct_idx, tid);

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

    // Update the local predictor.
    lct_idx = getLocalIndex(inst_addr);

    if (prediction_outcome)
    {
        lctCtrs[lct_idx]++;
        DPRINTF(LVPUnit, "LCT: Update PC:0x%x (idx %u) cntr ++ %u\n", inst_addr, lct_idx, (uint8_t)lctCtrs[lct_idx]);
    }
    else
    {
       lctCtrs[lct_idx]--;
       DPRINTF(LVPUnit, "LCT: Update PC:0x%x (idx %u) cntr -- %u\n", inst_addr, lct_idx, (uint8_t)lctCtrs[lct_idx]);
    }
}

inline unsigned LCT::getLocalIndex(Addr inst_addr)
{
    return (inst_addr >> instShiftAmt) & indexMask;
}

} // namespace o3
} // namespace gem5
