#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/lvp_unit.hh"
#include <algorithm>
#include "arch/generic/pcstate.hh"
#include "base/compiler.hh"
#include "base/trace.hh"
#include "config/the_isa.hh"
#include "debug/LVPUnit.hh"

#include "params/BaseO3CPU.hh"

namespace gem5
{

namespace o3
{

LVPUnit::LVPUnit(const BaseO3CPUParams &params)
    : numThreads(params.numThreads),
        // predHist(numThreads),
        instShiftAmt(params.instShiftAmt),
        lct(params.LCTEntries,
            params.LCTCtrBits,
            instShiftAmt,
            params.numThreads),
        lvpt(params.LVPTEntries,
            instShiftAmt,
            params.numThreads),
        cvu(256, // hardcode it for now
            params.LVPTEntries, // for creating LVPT index
            instShiftAmt,
            params.numThreads)
        // stats(this)
{}


bool LVPUnit::predict(const DynInstPtr &inst)
{
    // See if LCT predicts predictible.
    // If so, get its value from LVPT.

    // ++stats.LCTLookups;

    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();
    uint8_t counter_val = lct.lookup(tid, pc.instAddr());
    bool is_predictible_ld = lct.getPrediction(counter_val);
    
    if (is_predictible_ld == false) 
    {
        DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu]  PC:0x%x Not Predictible %\n", inst->threadNumber, inst->seqNum, pc.instAddr());

        inst -> setLdPredictible(false);
        inst -> setLdConstant(false);
        inst -> PredictedLdValue(0);
        return false;
    }
    else
    {   
        inst -> setLdPredictible(true);
        if (counter_val == 3)
        { inst->setLdConstant(true); } 
        else
        { inst->setLdConstant(false); }

            if (lvpt.valid(pc.instAddr(), tid))
            {
                uint8_t ld_predict_val = lvpt.lookup(pc.instAddr(), tid);
                inst->PredictedLdValue(ld_predict_val);
                DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] LVP predicted for PC:0x%x with ld_val = %u\n", inst->threadNumber, inst->seqNum, pc.instAddr(), ld_predict_val);
                return true;
            }
            else
            {
                DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] **** LVPT and LCT outcome not matching **** PC:0x%x \n", tid, inst->seqNum, pc.instAddr());
                inst->PredictedLdValue(0);
                return false;
            }

        return true;
    }
}

void LVPUnit::update(const DynInstPtr &inst)
{
    //can use  effAddrValid()

    uint8_t mem_ld_value = *(inst->memData);
    const PCStateBase &pc = inst->pcState();
    ThreadID tid = inst->threadNumber;

    DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x data_addr:%llu ld_val:%u \n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr, *(inst->memData));

    bool valid_entry = lvpt.valid(pc.instAddr(), tid);

    if (!valid_entry)
    {
        lvpt.update(pc.instAddr(), mem_ld_value, tid);
        //make the counter to not predictible (00) => not predictible (01)
        lct.update(tid, pc.instAddr(), true, false);
    }
    else
    {       
        uint8_t lvpt_ld_value = lvpt.lookup(pc.instAddr(), tid);
        uint8_t pred_ld_value = inst->PredictedLdValue();

        if (lvpt_ld_value != pred_ld_value)
        {
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x **** LVPT ENTRY != PREDICTED VALUE *** lvpt_ld_value:%u pred_ld_value:%u  mem_ld_value=%u\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);
        }
        else
        {
            if (pred_ld_value == mem_ld_value)
            {
                // make the counter to predictible
                lct.update(tid, pc.instAddr(), true, false);

                if (lct.lookup(tid, pc.instAddr()) == 3)
                {
                    cvu.update(pc.instAddr(), inst->effAddr, mem_ld_value, tid);
                }
            }
            else
            {
                // make the counter to not predictible
                lct.update(tid, pc.instAddr(), false, false);
                lvpt.update(pc.instAddr(), mem_ld_value, tid);
            }
         }
    }
}

void LVPUnit::cvu_invalidate(const DynInstPtr &inst) {
    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr StdataAddr = inst->effAddr;
    ThreadID tid = inst->threadNumber;
    
    //DPRINTF(LVPUnit,"LVPUnit::cvu_invalidate stdataAddr:%llu\n",StdataAddr);
    cvu.invalidate(instPC,StdataAddr,tid);

    return;
}

bool LVPUnit::cvu_valid(const DynInstPtr &inst) {
    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr LwdataAddr = inst-> effAddr;
    ThreadID tid = inst->threadNumber;

    return cvu.valid(instPC,LwdataAddr,tid);
}

/*
LVPUnit::LVPUnitStats::LVPUnitStats(statistics::Group *parent)
    : statistics::Group(parent),
      ADD_STAT(lookups, statistics::units::Count::get(),
              "Number of BP lookups"),
      ADD_STAT(condPredicted, statistics::units::Count::get(),
               "Number of conditional branches predicted"),
      ADD_STAT(condIncorrect, statistics::units::Count::get(),
               "Number of conditional branches incorrect"),
      ADD_STAT(LVPTLookups, statistics::units::Count::get(),
               "Number of BTB lookups"),
      ADD_STAT(LVPTHits, statistics::units::Count::get(), "Number of BTB hits"),
      ADD_STAT(BTBHitRatio, statistics::units::Ratio::get(), "BTB Hit Ratio",
               LVPTHits / LVPTLookups),
      ADD_STAT(RASUsed, statistics::units::Count::get(),
               "Number of times the RAS was used to get a target."),
      ADD_STAT(RASIncorrect, statistics::units::Count::get(),
               "Number of incorrect RAS predictions."),
      ADD_STAT(indirectLookups, statistics::units::Count::get(),
               "Number of indirect predictor lookups."),
      ADD_STAT(indirectHits, statistics::units::Count::get(),
               "Number of indirect target hits."),
      ADD_STAT(indirectMisses, statistics::units::Count::get(),
               "Number of indirect misses."),
      ADD_STAT(indirectMispredicted, statistics::units::Count::get(),
               "Number of mispredicted indirect branches.")
{
    BTBHitRatio.precision(6);
}
void LVPUnit::drainSanityCheck() const
{
    // We shouldn't have any outstanding requests when we resume from a drained system.
    for ([[maybe_unused]] const auto& ph : predHist)
        assert(ph.empty());
}

void LVPUnit::squash(const InstSeqNum &squashed_sn, ThreadID tid)
{
    History &pred_hist = predHist[tid];

    while (!pred_hist.empty() && pred_hist.front().seqNum > squashed_sn) {

        // This call should delete the ldHistory.
        squash(tid, pred_hist.front().ldHistory);

        DPRINTF(LVPUnit, "[tid:%i] [squash sn:%llu] Removing history for [sn:%llu] PC %#x\n", tid, squashed_sn, pred_hist.front().seqNum, pred_hist.front().pc);

        pred_hist.pop_front();

        DPRINTF(LVPUnit, "[tid:%i] [squash sn:%llu] predHist.size(): %i\n", tid, squashed_sn, predHist[tid].size());
    }
}

void LVPUnit::squash(const InstSeqNum &squashed_sn, const uint8_t &corr_ldval, bool actually_predictible, ThreadID tid)
{
    // Now that we know that a ldval was mispredicted, we need to undo
    // all the branches that have been seen up until this branch and
    // fix up everything.
    // NOTE: This should be call conceivably in 2 scenarios:
    // (1) After an branch is executed, it updates its status in the ROB
    //     The commit stage then checks the ROB update and sends a signal to
    //     the fetch stage to squash history after the mispredict
    // (2) In the decode stage, you can find out early if a unconditional
    //     PC-relative, branch was predicted incorrectly. If so, a signal
    //     to the fetch stage is sent to squash history after the mispredict

    History &pred_hist = predHist[tid];

    ++stats.ldvalIncorrect;

    DPRINTF(LVPUnit, "[tid:%i] Squashing from sequence number %i, setting target to %s\n", tid, squashed_sn, corr_target);

    // Squash All Branches AFTER this mispredicted branch
    squash(squashed_sn, tid);

    // If there's a squash due to a syscall, there may not be an entry
    // corresponding to the squash.  In that case, don't bother trying to
    // fix up the entry.
    if (!pred_hist.empty()) {

        auto hist_it = pred_hist.begin();
        //HistoryIt hist_it = find(pred_hist.begin(), pred_hist.end(),
        //                       squashed_sn);

        //assert(hist_it != pred_hist.end());
        if (pred_hist.front().seqNum != squashed_sn) {
            DPRINTF(LVPUnit, "Front sn %i != Squash sn %i\n",
                    pred_hist.front().seqNum, squashed_sn);

            assert(pred_hist.front().seqNum == squashed_sn);
        }


        if ((*hist_it).usedRAS) {
            ++stats.RASIncorrect;
            DPRINTF(LVPUnit,
                    "[tid:%i] [squash sn:%llu] Incorrect RAS [sn:%llu]\n",
                    tid, squashed_sn, hist_it->seqNum);
        }

        // There are separate functions for in-order and out-of-order
        // branch prediction, but not for update. Therefore, this
        // call should take into account that the mispredicted branch may
        // be on the wrong path (i.e., OoO execution), and that the counter
        // counter table(s) should not be updated. Thus, this call should
        // restore the state of the underlying predictor, for instance the
        // local/global histories. The counter tables will be updated when
        // the branch actually commits.

        // Remember the correct direction for the update at commit.
        pred_hist.front().predTaken = actually_taken;
        pred_hist.front().target = corr_target.instAddr();

        update(tid, (*hist_it).pc, actually_taken,
               pred_hist.front().bpHistory, true, pred_hist.front().inst,
               corr_target.instAddr());

        if (iPred) {
            iPred->changeDirectionPrediction(tid,
                pred_hist.front().indirectHistory, actually_taken);
        }

        if (actually_taken) {
            if (hist_it->wasReturn && !hist_it->usedRAS) {
                 DPRINTF(LVPUnit, "[tid:%i] [squash sn:%llu] "
                        "Incorrectly predicted "
                        "return [sn:%llu] PC: %#x\n", tid, squashed_sn,
                        hist_it->seqNum,
                        hist_it->pc);
                 RAS[tid].pop();
                 hist_it->usedRAS = true;
            }
            if (hist_it->wasIndirect) {
                ++stats.indirectMispredicted;
                if (iPred) {
                    iPred->recordTarget(
                        hist_it->seqNum, pred_hist.front().indirectHistory,
                        corr_target, tid);
                }
            } else {
                DPRINTF(LVPUnit,"[tid:%i] [squash sn:%llu] "
                        "BTB Update called for [sn:%llu] "
                        "PC %#x\n", tid, squashed_sn,
                        hist_it->seqNum, hist_it->pc);

                LVPT.update(hist_it->pc, corr_target, tid);
            }
        } else {
           //Actually not Taken
           if (hist_it->usedRAS) {
                DPRINTF(LVPUnit,
                        "[tid:%i] [squash sn:%llu] Incorrectly predicted "
                        "return [sn:%llu] PC: %#x Restoring RAS\n", tid,
                        squashed_sn,
                        hist_it->seqNum, hist_it->pc);
                DPRINTF(LVPUnit,
                        "[tid:%i] [squash sn:%llu] Restoring top of RAS "
                        "to: %i, target: %s\n", tid, squashed_sn,
                        hist_it->RASIndex, *hist_it->RASTarget);
                RAS[tid].restore(hist_it->RASIndex, hist_it->RASTarget.get());
                hist_it->usedRAS = false;
           } else if (hist_it->wasCall && hist_it->pushedRAS) {
                 //Was a Call but predicated false. Pop RAS here
                 DPRINTF(LVPUnit,
                        "[tid:%i] [squash sn:%llu] "
                        "Incorrectly predicted "
                        "Call [sn:%llu] PC: %s Popping RAS\n",
                        tid, squashed_sn,
                        hist_it->seqNum, hist_it->pc);
                 RAS[tid].pop();
                 hist_it->pushedRAS = false;
           }
        }
    } else {
        DPRINTF(LVPUnit, "[tid:%i] [sn:%llu] pred_hist empty, can't "
                "update\n", tid, squashed_sn);
    }
}

void LVPUnit::dump()
{
    int i = 0;
    for (const auto& ph : predHist) {
        if (!ph.empty()) {
            auto pred_hist_it = ph.begin();

            cprintf("predHist[%i].size(): %i\n", i++, ph.size());

            while (pred_hist_it != ph.end()) {
                cprintf("sn:%llu], PC:%#x, tid:%i, predTaken:%i, "
                        "bpHistory:%#x\n",
                        pred_hist_it->seqNum, pred_hist_it->pc,
                        pred_hist_it->tid, pred_hist_it->predPredictible,
                        pred_hist_it->ldHistory);
                pred_hist_it++;
            }

            cprintf("\n");
        }
    }
}

*/

} // namespace o3
} // namespace gem5
