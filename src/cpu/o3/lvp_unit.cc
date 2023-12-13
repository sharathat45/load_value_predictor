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

LVPUnit::LVPUnit(CPU *_cpu, const BaseO3CPUParams &params)
    : SimObject(params),
        numThreads(params.numThreads),
        // predHist(numThreads),
        stats(_cpu),
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
{}


bool LVPUnit::predict(const DynInstPtr &inst)
{
    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();

    // Lookup the prediction state for this instruction in the LCT and check if it's predictable
    uint8_t counter_val = lct.lookup(tid, pc.instAddr());
    bool is_predictable = lct.getPrediction(counter_val);
    inst->setLdPredictible(is_predictable);
    inst->setLdConstant(counter_val == 3);

    stats.LCTLookups += 1;
    stats.LCTPredictable += is_predictable;

    // Lookup the predicted load value for this instruction
    uint64_t pred_ld_val = lvpt.lookup(pc.instAddr(), tid);
    inst->PredictedLdValue(pred_ld_val);

    stats.LVPTLookups++;

    // Debug stuff
    if (is_predictable == false) 
    {
        DPRINTF(LVPUnit, "lvp_predict: [tid:%i] [sn:%llu] PC:0x%x LCT:%u Not Predictable\n",
                inst->threadNumber, inst->seqNum, pc.instAddr(), counter_val);
        return false;
    }
    else
    {   
        if (lvpt.valid(pc.instAddr(), tid))
        {
            ++stats.LVPTHits;
            ++stats.ldvalPredicted;
    
            DPRINTF(LVPUnit, "lvp_predict: [tid:%i] [sn:%llu] PC:0x%x LCT:%u LVPT:0x%x Predictable\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), counter_val, pred_ld_val);
            return true;
        }
        else       
        {
            DPRINTF(LVPUnit, "lvp_predict: [tid:%i] [sn:%llu] PC:0x%x LCT:%u LCT valid but LVPT invalid\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), counter_val);
            return false;
        }
    }
}

void LVPUnit::update(const DynInstPtr &inst)
{
    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();
    uint64_t mem_ld_value = *(inst->memData);

    // Lookup this instruction in the LVPT
    bool valid_lvpt_entry = lvpt.valid(pc.instAddr(), tid);

    if (!valid_lvpt_entry)
    {
        // If the LVPT was invalid before this, this is a fresh entry
        DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x data_addr:0x%x data_val:0x%x Fresh entry\n",
                inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr, mem_ld_value);

        lvpt.update(pc.instAddr(), mem_ld_value, tid);
        lct.update(tid, pc.instAddr(), true, false);
    }
    else
    {       
        uint64_t lvpt_ld_value = lvpt.lookup(pc.instAddr(), tid);
        uint64_t pred_ld_value = inst->PredictedLdValue();

        if (lvpt_ld_value != pred_ld_value)
        {
            // If the prediction in the LVPT doesn't match the prediction used by this instruction, some aliasing must have happened
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:0x%x pred_ld_value:0x%x  mem_ld_value=0x%x LVPT ENTRY != PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);
        }
        else
        {
            // If the prediction in the LVPT matches the prediction used by the instruction, we're good
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:0x%x pred_ld_value:0x%x  mem_ld_value=0x%x LVPT ENTRY == PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);

            if (pred_ld_value == mem_ld_value)
            {
                // Increment the LCT's saturating counter
                lct.update(tid, pc.instAddr(), true, false);

                // If the entry for this instruction in the LCT became constant, update the CVU to reflect this
                if (lct.lookup(tid, pc.instAddr()) == 3)
                {
                    cvu.update(pc.instAddr(), inst->effAddr, mem_ld_value, tid);
                }
            }
            else
            {
                // Decrement the LCT's saturating counter
                lct.update(tid, pc.instAddr(), false, false);

                // If the entry for this instruction in the LCT became 0, update the prediction in the LVPT
                if (lct.lookup(tid, pc.instAddr()) == 0)
                {
                    lvpt.update(pc.instAddr(), mem_ld_value, tid);
                }

                stats.ldvalIncorrect++;
            }
         }
    }
}

void LVPUnit::cvu_invalidate(const DynInstPtr &inst) {
    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr StdataAddr = inst->effAddr;
    ThreadID tid = inst->threadNumber;
    
    DPRINTF(LVPUnit, "cvu_invalidate: [tid:%i] [sn:%llu] PC:0x%x data_addr:0x%x\n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr);

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


LVPUnit::LVPUnitStats::LVPUnitStats(statistics::Group *parent)
    : statistics::Group(parent, "lvp"),
      ADD_STAT(ldvalPredicted, statistics::units::Count::get(),
               "Number of load values predicted"),
      ADD_STAT(ldvalIncorrect, statistics::units::Count::get(),
               "Number of load values incorrect"),
      ADD_STAT(ldvalAccuracy, statistics::units::Ratio::get(),
               "Fraction of predicted load values that were correctly predicted",
               (ldvalPredicted - ldvalIncorrect) / ldvalPredicted),
      ADD_STAT(LCTLookups, statistics::units::Count::get(),
               "Number of LCT lookups"),
      ADD_STAT(LCTPredictable, statistics::units::Count::get(),
               "Number of LCT lookups that were predictable"),
      ADD_STAT(LVPTLookups, statistics::units::Count::get(),
               "Number of LVPT lookups"),
      ADD_STAT(LVPTHits, statistics::units::Count::get(), "Number of LVPT hits"),
      ADD_STAT(LVPTHitRatio, statistics::units::Ratio::get(), "LVPT Hit Ratio",
               LVPTHits / LVPTLookups)
{
    LVPTHitRatio.precision(6);
}

} // namespace o3
} // namespace gem5
