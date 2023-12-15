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
    ++stats.lookups;

    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();

    // Lookup the predicted load value for this instruction
    uint64_t pred_ld_val = lvpt.lookup(pc.instAddr(), tid);
    bool lvpt_valid = lvpt.valid(pc.instAddr(), tid);
    inst->PredictedLdValue(pred_ld_val);

    // Lookup the prediction state for this instruction in the LCT and check if it's predictable
    uint8_t counter_val = lct.lookup(tid, pc.instAddr());
    bool is_predictable = lct.getPrediction(counter_val);
    inst->setLdPredictible(is_predictable && lvpt_valid);
    inst->setLdConstant(counter_val == 3 && lvpt_valid);

    // Debug stuff
    if (is_predictable == false) 
    {
        DPRINTF(LVPUnit, "lvp_predict: [tid:%i] [sn:%llu] PC:0x%x LCT:%u Not Predictable\n",
                inst->threadNumber, inst->seqNum, pc.instAddr(), counter_val);
        return false;
    }
    else
    {   
        if (lvpt_valid)
        {
            ++stats.predTotal;

            if (counter_val == 3) {
                ++stats.constPred;
            }
    
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
    uint64_t mem_ld_value = *((uint64_t*)(inst->memData));

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

        if (pred_ld_value == mem_ld_value)
        {
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:0x%x pred_ld_value:0x%x  mem_ld_value=0x%x MEM VALUE == PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);

            // Increment the LCT's saturating counter
            lct.update(tid, pc.instAddr(), true, false);

            // If the entry for this instruction in the LCT became constant, update the CVU to reflect this
            if (lct.lookup(tid, pc.instAddr()) == 3)
            {
                cvu.update(pc.instAddr(), inst->effAddr, inst->effSize, mem_ld_value, tid);
            }

            stats.predCorrect += inst->readLdPredictible();
        }
        else
        {
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:0x%x pred_ld_value:0x%x  mem_ld_value=0x%x MEM VALUE != PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);

            // Decrement the LCT's saturating counter
            lct.update(tid, pc.instAddr(), false, false);

            // If the entry for this instruction in the LCT became 0, update the prediction in the LVPT
            if (lct.lookup(tid, pc.instAddr()) == 0)
            {
                lvpt.update(pc.instAddr(), mem_ld_value, tid);
            }

            stats.predIncorrect += inst->readLdPredictible();
            stats.constRollback += inst->readLdConstant();
        }
    }
}

void LVPUnit::cvu_invalidate(const DynInstPtr &inst) {
    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr StdataAddr = inst->effAddr;
    Addr St_effsize = inst->effSize;
    ThreadID tid = inst->threadNumber;
    
    DPRINTF(LVPUnit, "cvu_invalidate: [tid:%i] [sn:%llu] PC:0x%x data_addr:0x%x\n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr);

    if (cvu.invalidate(instPC,StdataAddr,St_effsize,tid)) {
        // For now if we had a CVU invalidation, downgrade the LCT prediction counter
        lct.update(tid, pc.instAddr(), false, false);

        ++stats.constInval;
    }

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
      // LVP overall stats
      ADD_STAT(lookups, statistics::units::Count::get(),
               "Total number of LVP lookups"),
      ADD_STAT(predTotal, statistics::units::Count::get(),
               "Total number of loads predicted"),
      ADD_STAT(predCorrect, statistics::units::Count::get(),
               "Total number of loads predicted correctly"),
      ADD_STAT(predIncorrect, statistics::units::Count::get(),
               "Total number of loads predicted incorrectly"),
      ADD_STAT(predRate, statistics::units::Ratio::get(),
               "Fraction of loads that resulted in a prediction",
               predTotal / lookups),
      ADD_STAT(predAccuracy, statistics::units::Ratio::get(),
               "Fraction of predicted loads that were correctly predicted",
               predCorrect / (predCorrect + predIncorrect)),
      // Constant prediction stats
      ADD_STAT(constPred, statistics::units::Count::get(),
               "Total number of loads predicted constant"),
      ADD_STAT(constInval, statistics::units::Count::get(),
               "Total number of constant entries invalidated"),
      ADD_STAT(constRollback, statistics::units::Count::get(),
               "Total number of constant loads requiring rollback"),
      ADD_STAT(constRollbackRate, statistics::units::Ratio::get(),
               "Fraction of predicted loads that were correctly predicted",
               constRollback / constPred)
{ }

} // namespace o3
} // namespace gem5
