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

    bool predict(const DynInstPtr &inst);

    void update(const DynInstPtr &inst, uint64_t mem_ld_value);

    void cvu_invalidate(const DynInstPtr &inst);

    bool cvu_valid(const DynInstPtr &inst);


  private:
    const unsigned numThreads;

    struct LVPUnitStats : public statistics::Group
    {
        LVPUnitStats(statistics::Group *parent);

        /** Stat for number of lds predicted. */
        statistics::Scalar ldvalPredicted;
        /** Stat for number of lds predicted incorrectly. */
        statistics::Scalar ldvalIncorrect;
        /** Stat for fraction of predicted lds predicted correctly. */
        statistics::Formula ldvalAccuracy;
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

    uint32_t numEntries;

    struct LVPEntry
    {
        bool valid = false;
        uint8_t cntr = 0;
        Addr data_addr;
        uint64_t data_value;
    };

    std::vector<LVPEntry> lvp_table;
    unsigned indexMask;

    inline unsigned getIndex(Addr inst_addr);

};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_LVP_UNIT_HH__
