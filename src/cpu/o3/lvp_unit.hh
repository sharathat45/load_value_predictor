#ifndef __CPU_O3_LVP_UNIT_HH__
#define __CPU_O3_LVP_UNIT_HH__

#include <deque>

#include "base/statistics.hh"
#include "base/types.hh"

#include "cpu/o3/lvpt.hh"
#include "cpu/o3/2bit_lct.hh"
#include "cpu/o3/cvu.hh"

#include "cpu/inst_seq.hh"
#include "cpu/static_inst.hh"
#include "sim/sim_object.hh"

#include "params/BaseO3CPU.hh"

#include "cpu/o3/dyn_inst_ptr.hh"

namespace gem5
{

namespace o3
{

/**
 * Basically a wrapper class to hold LCT, LVPT and CVU
 */
class LVPUnit : public SimObject
{
  public:
    
    LVPUnit(CPU *_cpu, const BaseO3CPUParams &params);

    /** Perform sanity checks after a drain. */
    // void drainSanityCheck() const;

    /**
     * Predicts whether or not the ld instruction is predictible or not, and the value of the ld instruction if it is predictible.
     * @param inst The ld instruction.
     * @param ld_Value The predicted value is passed back through this parameter.
     * @param tid The thread id.
     * @return Returns if the ld is predictible or not.
     */
    bool predict(const DynInstPtr &inst);

    /**
     * Tells the LCT to commit any updates until the given sequence number.
     * @param done_sn The sequence number to commit any older updates up until.
     * @param tid The thread id.
     */
    void update(const DynInstPtr &inst);

    void cvu_invalidate(const DynInstPtr &inst);

    bool cvu_valid(const DynInstPtr &inst);

    /**
     * Squashes all outstanding updates until a given sequence number.
     * @param squashed_sn The sequence number to squash any younger updates up
     * until.
     * @param tid The thread id.
     */
    // void squash(const InstSeqNum &squashed_sn, ThreadID tid);

    /**
     * Squashes all outstanding updates until a given sequence number, and
     * corrects that sn's update with the proper ld val and predictible/not.
     * @param squashed_sn The sequence number to squash any younger updates up
     * until.
     * @param corr_ldval The correct ld value.
     * @param actually_predictible The correct lvp direction.
     * @param tid The thread id.
     */
    // void squash(const InstSeqNum &squashed_sn, const uint8_t &corr_ldval, bool actually_predictible, ThreadID tid);
        
    // void dump();

  private:
    const unsigned numThreads;

    struct LVPUnitStats : public statistics::Group
    {
        LVPUnitStats(statistics::Group *parent);

        /** Stat for number of lds predicted. */
        statistics::Scalar ldvalPredicted;
        /** Stat for number of lds predicted incorrectly. */
        statistics::Scalar ldvalIncorrect;
        /** Stat for number of LCT lookups. */
        statistics::Scalar LCTLookups;
        /** Stat for number of LCT predictable lookups. */
        statistics::Scalar LCTPredictable;
        /** Stat for number of LVPT lookups. */
        statistics::Scalar LVPTLookups;
        /** Stat for number of LVPT hits. */
        statistics::Scalar LVPTHits;
        /** Stat for the ratio between LVPT hits and LVPT lookups. */
        statistics::Formula LVPTHitRatio;
    } stats;

  protected:
    /** Number of bits to shift instructions by for predictor addresses. */
    const unsigned instShiftAmt;

  private:
    /** The LCT */
    LCT lct;

    /** The LVPT. */
    LVPT lvpt;
     
    /** The CVU */
    CVU cvu;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_LVP_UNIT_HH__
