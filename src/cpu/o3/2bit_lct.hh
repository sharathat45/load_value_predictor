#ifndef __CPU_O3_2BIT_LCT_HH__
#define __CPU_O3_2BIT_LCT_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"

namespace gem5
{

namespace o3
{

/**
 * Implements a local predictor that uses the PC to index into a table of
 * counters.  Note that any time a pointer to the ld_history is given, it
 * should be NULL using this predictor because it does not have any branch
 * predictor state that needs to be recorded or updated; the update can be
 * determined solely by the ld being predictible or not.
 */
class LCT 
{
  public:

    LCT(unsigned _lctSize, unsigned _lctCtrBits, unsigned _instShiftAmt, unsigned _numThreads);

    uint8_t lookup(ThreadID tid, Addr inst_addr);

    void lvptUpdate(ThreadID tid, Addr inst_addr, void * &ld_history);

    void update(ThreadID tid, Addr inst_addr, bool prediction_outcome, bool squashed);

    void squash(ThreadID tid, void *ld_history)
    { assert(ld_history == NULL); }

    bool getPrediction(uint8_t &count);

  private:
   
    inline unsigned getLocalIndex(Addr inst_addr);
    const unsigned lctSize;
    const unsigned lctCtrBits;
    const unsigned lctPredictorSets;
    std::vector<SatCounter8> lctCtrs;
    const unsigned indexMask;
    const unsigned instShiftAmt;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_2BIT_LCT_HH__
