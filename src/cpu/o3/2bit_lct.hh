#ifndef __CPU_O3_2BIT_LCT_HH__
#define __CPU_O3_2BIT_LCT_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/o3/lvp_unit.hh"

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
class LCT : public LVPUnit
{
  public:
    /**
     * Default LCT constructor.
     */
    LCT(const BaseO3CPUParams &params);

    /**
     * Looks up the given address in the LCT and returns
     * a true/false value as to whether it is taken.
     * @param ld_addr The address of the ld to look up.
     * @param ld_history Pointer to any ld history state.
     * @return Whether or not the branch is taken.
     */
    bool lookup(ThreadID tid, Addr ld_addr, void * &ld_history);

    /**
     * Updates the LCT to don't predict if a lvpt entry is
     * invalid or not found.
     * @param ld_addr The address of the ld to look up.
     * @param ld_history Pointer to any ld history state.
     * @return Whether or not the ld is predictible.
     */
    void lvptUpdate(ThreadID tid, Addr ld_addr, void * &ld_history);

    /**
     * Updates the branch predictor with the actual result of a branch.
     * @param ld_addr The address of the ld to update.
     * @param taken Whether or not the ld was predictible.
     */
    void update(ThreadID tid, Addr ld_addr, bool predictible, void *ld_history,
                bool squashed, const StaticInstPtr &inst, Addr corrTarget);

    void squash(ThreadID tid, void *ld_history)
    { assert(ld_history == NULL); }

  private:
    /**
     *  Returns the predictible or not prediction given the value of the
     *  counter.
     *  @param count The value of the counter.
     *  @return The prediction based on the counter value.
     */
    inline bool getPrediction(uint8_t &count);

    /** Calculates the local index based on the PC. */
    inline unsigned getLocalIndex(Addr &PC);

    /** Size of the local predictor. */
    const unsigned lctSize;

    /** Number of bits of the local predictor's counters. */
    const unsigned lctCtrBits;

    /** Number of sets. */
    const unsigned lctPredictorSets;

    /** Array of counters that make up the local predictor. */
    std::vector<SatCounter8> lctCtrs;

    /** Mask to get index bits. */
    const unsigned indexMask;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_2BIT_LCT_HH__
