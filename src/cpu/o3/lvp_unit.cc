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
    bool is_predictible_ld = lct.getPrediction(counter_val);
    //uint64_t ld_predict_val = lvpt.lookup(pc.instAddr(), tid);

    ++stats.ldvalPredicted;

    DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu]  PC:0x%x\n", inst->threadNumber, inst->seqNum, pc.instAddr());
    
    if (is_predictible_ld == false) 
    {
        DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu]  PC:0x%x Not Predictible %\n", inst->threadNumber, inst->seqNum, pc.instAddr());

        inst -> setLdPredictible(false);
        inst -> setLdConstant(false);
        if(lvpt.valid(pc.instAddr(), tid)) {
            inst -> PredictedLdValue(lvpt.lookup(pc.instAddr(), tid));  
            //inst -> PredictedLdValue(0);
        }
        else {
            inst -> PredictedLdValue(-1);
        }
        return false;
    }
    else
    {   
        ++stats.LCTPredictable;
        inst -> setLdPredictible(true);
        if (counter_val == 3)
        { inst->setLdConstant(true); } 
        else
        { inst->setLdConstant(false); }

        if (lvpt.valid(pc.instAddr(), tid))
        {
            uint64_t ld_predict_val = lvpt.lookup(pc.instAddr(), tid);
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
    assert(inst->memData);

    uint64_t mem_ld_value = *(inst->memData);
    const PCStateBase &pc = inst->pcState();
    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();
    uint64_t mem_ld_value = *((uint64_t*)(inst->memData));

    DPRINTF(LVPUnit, "lvp_update(called in writeback): [tid:%i] [sn:%llu] PC:0x%x data_addr:%x ld_val:%u \n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr, *(inst->memData));

    bool valid_entry = lvpt.valid(pc.instAddr(), tid);
    unsigned lct_pre_update;

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

        // if (lvpt_ld_value != pred_ld_value)
        // {
        //     DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x **** LVPT ENTRY != PREDICTED VALUE *** lvpt_ld_value:%u pred_ld_value:%u  mem_ld_value=%u\n",
        //             inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);
        // }
        // else
        // {
        if ((pred_ld_value == mem_ld_value))
        {   
            if(pred_ld_value == 0) {
                ++stats.ldvalPredicted_0;
            }

            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x pred_ld_value:%llu mem_ld_value:%u \n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), mem_ld_value, mem_ld_value);
            // make the counter to predictible
            lct_pre_update = lct.lookup(tid, pc.instAddr());
            lct.update(tid, pc.instAddr(), true, false);
            //lvpt.update(pc.instAddr(), mem_ld_value, tid);

            if (lct_pre_update == 2 && lct.lookup(tid, pc.instAddr()) == 3)
            {
                cvu.update(pc.instAddr(), inst->effAddr, inst->effSize, mem_ld_value, tid);
            }
        }

        if (pred_ld_value == mem_ld_value)
        {
            // make the counter to not predictible
            lct.update(tid, pc.instAddr(), false, false);
            lvpt.update(pc.instAddr(), mem_ld_value, tid);

            stats.ldvalIncorrect++;
        }
         //}
    }
}

void LVPUnit::ldconst_check(const DynInstPtr &inst) {
    uint64_t mem_ld_value = *(inst->memData);
    uint64_t pred_ld_value = inst->PredictedLdValue();
    const PCStateBase &pc = inst->pcState();

    if (pred_ld_value != mem_ld_value) {
        stats.ldvalIncorrect++;
        stats.wrongldConst++;
        
        DPRINTF(LVPUnit, "ldconst_check wrong: [tid:%i] [sn:%llu] PC:0x%x, ld_addr:0x%x, pred_ld_value:%llu mem_ld_value:%llu\n",
            inst->threadNumber, inst->seqNum, pc.instAddr(),inst->effAddr, pred_ld_value, mem_ld_value);
    }
    else {
        DPRINTF(LVPUnit, "ldconst_check correct: [tid:%i] [sn:%llu] PC:0x%x, ld_addr:0x%x, pred_ld_value:%llu mem_ld_value:%llu\n",
            inst->threadNumber, inst->seqNum, pc.instAddr(),inst->effAddr, pred_ld_value, mem_ld_value);
    }

    if (pred_ld_value == 0) {
        stats.ldvalPredicted_0++;
        stats.ldConst_as_zero++;
    }
    stats.totalldConst++;

    return;

}

void LVPUnit::cvu_invalidate(const DynInstPtr &inst) {
    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr StdataAddr = inst->effAddr;
    Addr St_effsize = inst->effSize;
    ThreadID tid = inst->threadNumber;
    
    //DPRINTF(LVPUnit,"LVPUnit::cvu_invalidate stdataAddr:%llu\n",StdataAddr);
    cvu.invalidate(instPC,StdataAddr,St_effsize,tid);

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
       ADD_STAT(wrongldConst, statistics::units::Count::get(),
               "Number of load values predicted as constld but turns out to be wrong"),
      //totalldConst
      ADD_STAT(totalldConst, statistics::units::Count::get(),
               "Total Number of load values predicted as constld"),
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
