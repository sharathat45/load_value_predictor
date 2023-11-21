#include "cpu/o3/2bit_lct.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
// #include "debug/Fetch.hh"

namespace gem5
{

namespace o3
{

LCT::LCT(const BaseO3CPUParams &params)
    : lctSize(params.lctSize),
      lctCtrBits(params.lctCtrBits),
      lctPredictorSets(lctSize / lctCtrBits),
      lctCtrs(lctPredictorSets, SatCounter8(lctCtrBits)),
      indexMask(lctPredictorSets - 1)
{
    if (!isPowerOf2(lctSize)) {
        fatal("Invalid LCT size!\n");
    }

    if (!isPowerOf2(lctPredictorSets)) {
        fatal("Invalid number of LCT predictor sets! Check lctCtrBits.\n");
    }

    DPRINTF(Fetch, "index mask: %#x\n", indexMask); 

    DPRINTF(Fetch, "local predictor size: %i\n", lctSize); 

    DPRINTF(Fetch, "local counter bits: %i\n", lctCtrBits); 

    DPRINTF(Fetch, "instruction shift amount: %i\n", instShiftAmt); 
}

void LCT::lvptUpdate(ThreadID tid, Addr ld_addr, void *&ld_history)
{
    // Place holder for a function that is called to update predictor history when
    // a lvptUpdate entry is invalid or not found.
}

bool LCT::lookup(ThreadID tid, Addr ld_addr, void *&ld_history)
{
    bool predictible;
    unsigned lct_idx = getLocalIndex(ld_addr);

    DPRINTF(Fetch, "Looking up index %#x\n", lct_idx); 

    uint8_t counter_val = lctCtrs[lct_idx];

    DPRINTF(Fetch, "prediction is %i.\n", (int)counter_val); 

    predictible = getPrediction(counter_val);

    return predictible;
}

void LCT:: update(ThreadID tid, Addr ld_addr, bool predictible, void *ld_history, bool squashed, const StaticInstPtr & inst, Addr ldval)
{
    assert(ld_history == NULL);
    unsigned lct_idx;

    // No state to restore, and we do not update on the wrong path.
    if (squashed) {
        return;
    }

    // Update the local predictor.
    lct_idx = getLocalIndex(ld_addr);

    DPRINTF(Fetch, "Looking up index %#x\n", lct_idx);

    if (predictible)
    {
        DPRINTF(Fetch, "ld ins address updated as predictible.\n");
        lctCtrs[lct_idx]++;
    }
    else
    {
        DPRINTF(Fetch, "ld ins address updated as not predictible.\n");
        lctCtrs[lct_idx]--;
    }
}

inline bool LCT::getPrediction(uint8_t &count)
{
    // Get the MSB of the count
    return (count >> (lctCtrBits - 1));
}

inline unsigned LCT::getLocalIndex(Addr &ld_addr)
{
    return (ld_addr >> instShiftAmt) & indexMask;
}

} // namespace o3
} // namespace gem5
