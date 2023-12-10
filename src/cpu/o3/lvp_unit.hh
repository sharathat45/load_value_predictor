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
 * Basically a wrapper class to hold LCT, LVPT and CVT
 */
class LVPUnit : public SimObject
{
  public:
    
    LVPUnit(const BaseO3CPUParams &params);

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

    /**
     * @param ld_history Pointer to the history object.  The predictor
     * will need to update any state and delete the object.
     */
    // virtual void squash(ThreadID tid, void *ld_history) = 0;

    /**
     * Looks up a given PC in the BP to see if it is taken or not taken.
     * @param ld_addr The PC to look up.
     * @param ld_history Pointer that will be set to an object that
     * has the branch predictor state associated with the lookup.
     * @return Whether the branch is taken or not taken.
     */
    // virtual bool lookup(ThreadID tid, Addr ld_addr, void *&ld_history) = 0;

    /**
     * If a ld is not predictible, because the LVPT address is invalid or missing,
     * this function sets the appropriate counter in the global and local
     * predictors to not predictible.
     * @param inst_PC The PC to look up the local predictor.
     * @param ld_history Pointer that will be set to an object that
     * has the ld predictor state associated with the lookup.
     */
    // virtual void lvptUpdate(ThreadID tid, Addr ld_addr, void *&ld_history) = 0;

    /**
     * Looks up a given ld ins addr in the LVPT to see if a matching entry exists.
     * @param inst_PC The PC to look up.
     * @return Whether the LVPT contains the given PC.
     */
    // bool LVPTValid(Addr instPC) { return LVPT.valid(instPC, 0); }

    /**
     * Looks up a given PC in the LVPT to get the predicted ld value. The PC may
     * be changed or deleted in the future, so it needs to be used immediately,
     * and/or copied for use later.
     * @param inst_PC The PC to look up.
     * @return The ld value.
     */
    // const uint8_t * LVPTLookup(Addr inst_pc)
    // {
    //     return LVPT.lookup(inst_pc, 0);
    // }

    /**
     * Updates the LCT with predictible/not  information.
     * @param inst_PC The ld PC that will be updated.
     * @param predictible Whether the ld was predictible or not.
     * @param ld_history Pointer to the ld predictor state that is
     * associated with the ld lookup that is being updated.
     * @param squashed Set to true when this function is called during a
     * squash operation.
     * @param inst Static instruction information
     * @param ldval The actual ld value(only needed for squashed lds)
     * @todo Make this update flexible enough to handle a global predictor.
     */
    // virtual void update(ThreadID tid, Addr ld_addr, bool predictible, void *ld_history, bool squashed, const StaticInstPtr &inst, Addr ldval) = 0;

    /**
     * Updates the LVPT with the ldval of a ld ins.
     * @param inst_PC The ld ins PC that will be updated.
     * @param ldval The ld value that will be added to the LVPT.
     */
    // void LVPTUpdate(Addr instPC, const uint8_t &ldval)
    // {
    //     LVPT.update(instPC, target, 0);
    // }

    // void dump();

  private:
    //   struct PredictorHistory
    //   {
    //       /**
    //        * Makes a predictor history struct that contains any
    //        * information needed to update the LCT and LVPT
    //        */
    //       PredictorHistory(const InstSeqNum &seq_num, Addr instPC, bool pred_predictible_ld, void *ld_history, ThreadID _tid, const StaticInstPtr &inst)
    //           : seqNum(seq_num), pc(instPC), ldHistory(ld_history), tid(_tid), predPredictible(pred_predictible_ld), inst(inst)
    //       {
    //       }

    //       PredictorHistory(const PredictorHistory &other) : seqNum(other.seqNum), pc(other.pc), ldHistory(other.ldHistory),
    //                                                         tid(other.tid), predPredictible(other.predPredictible),
    //                                                         ldval(other.ldval), inst(other.inst)
    //       {
    //       }

    //       bool
    //       operator==(const PredictorHistory &entry) const
    //       {
    //           return this->seqNum == entry.seqNum;
    //       }

    //       /** The sequence number for the predictor history entry. */
    //       InstSeqNum seqNum;

    //       /** The PC associated with the sequence number. */
    //       Addr pc;

    //       /** Pointer to the history object passed back from the ld
    //        * predictor.  It is used to update or restore state of the
    //        * ld predictor.
    //        */
    //       void *ldHistory = nullptr;

    //       /** The thread id. */
    //       ThreadID tid;

    //       /** Whether or not it was predictible ld. */
    //       bool predPredictible;

    //       /** ld value of the ld ins, First it is predicted, and fixed later if necessary*/
    //       Addr ldval = MaxAddr;

    //       /** The ld instrction */
    //       const StaticInstPtr inst;
    //   };

    //   typedef std::deque<PredictorHistory> History;

    /** Number of the threads for which the branch history is maintained. */
    const unsigned numThreads;

    /**
     * The per-thread predictor history. This is used to update the predictor
     * as instructions are committed, or restore it to the proper state after
     * a squash.
     */
    //   std::vector<History> predHist;

    struct LVPUnitStats : public statistics::Group
    {
        LVPUnitStats(statistics::Group *parent);

        /** Stat for number of LVP lookups. */
        statistics::Scalar lookups;
        /** Stat for number of lds predicted. */
        statistics::Scalar ldvalPredicted;
        /** Stat for number of lds predicted incorrectly. */
        statistics::Scalar ldvalIncorrect;
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
      /** The LVPT. */
      LVPT lvpt;

      /** The LCT */
      LCT lct;

  public: // Need to call CVU invalidation in iew.cc
      /** The CVU */
      CVU cvu;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_LVP_UNIT_HH__
