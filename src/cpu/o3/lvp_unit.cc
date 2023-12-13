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
    // See if LCT predicts predictible.
    // If so, get its value from LVPT.

    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();
    uint8_t counter_val = lct.lookup(tid, pc.instAddr());
    bool is_predictible_ld = lct.getPrediction(counter_val);

    stats.LCTLookups += 1;
    stats.LCTPredictable += is_predictible_ld;
    
    if (is_predictible_ld == false) 
    {
        inst -> setLdPredictible(false);
        inst -> setLdConstant(false);
        inst -> PredictedLdValue(lvpt.lookup(pc.instAddr(), tid));

        DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu]  PC:0x%x Not Predictible \n", inst->threadNumber, inst->seqNum, pc.instAddr());
        return false;
    }
    else
    {   
        ++stats.LVPTLookups;

        if (lvpt.valid(pc.instAddr(), tid))
        {
            ++stats.LVPTHits;
            ++stats.ldvalPredicted;

            uint64_t ld_predict_val = lvpt.lookup(pc.instAddr(), tid);
            inst->PredictedLdValue(ld_predict_val);
            inst -> setLdConstant(counter_val == 3);
            inst -> setLdPredictible(true);
    
            DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] PC:0x%x ld_val = %llu LVP predicted predictible\n", inst->threadNumber, inst->seqNum, inst->pcState(), ld_predict_val);
            return true;
        }
        else       
        {
            inst -> PredictedLdValue(0);
            inst -> setLdConstant(false);
            inst -> setLdPredictible(false);

            DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] PC:0x%x **** LVPT and LCT outcome not matching **** \n", tid, inst->seqNum, pc.instAddr());
            return false;
        }

        return true;
    }
}

void LVPUnit::update(const DynInstPtr &inst)
{
    //can use  effAddrValid()

    uint64_t mem_ld_value = *(inst->memData);
    const PCStateBase &pc = inst->pcState();
    ThreadID tid = inst->threadNumber;


    bool valid_entry = lvpt.valid(pc.instAddr(), tid);

    if (!valid_entry)
    {
        DPRINTF(LVPUnit, "lvp_fresh_add: [tid:%i] [sn:%llu] PC:0x%x data_addr:%llu ld_val:%u \n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), inst->effAddr, *(inst->memData));

        lvpt.update(pc.instAddr(), mem_ld_value, tid);
        lct.update(tid, pc.instAddr(), true, false);
    }
    else
    {       
        uint64_t lvpt_ld_value = lvpt.lookup(pc.instAddr(), tid);
        uint64_t pred_ld_value = inst->PredictedLdValue();

        if (lvpt_ld_value != pred_ld_value)
        {
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:%llu pred_ld_value:%llu  mem_ld_value=%llu **** LVPT ENTRY != PREDICTED VALUE ***\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);
        }
        else
        {
            DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x lvpt_ld_value:%llu pred_ld_value:%llu  mem_ld_value=%llu LVPT ENTRY == PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), lvpt_ld_value, pred_ld_value, mem_ld_value);
    
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
