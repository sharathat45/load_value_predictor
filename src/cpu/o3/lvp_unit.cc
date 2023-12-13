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
            params.numThreads),
        numEntries(params.LVPTEntries),
        lvp_table(numEntries, LVPEntry())
{
    
    indexMask = numEntries - 1;

    for (unsigned i = 0; i < numEntries; ++i) {
        lvp_table[i].valid = false;
        lvp_table[i].cntr = 0;
        lvp_table[i].data_addr = 0;
        lvp_table[i].data_value = 0;
    }
}

inline unsigned LVPUnit::getIndex(Addr inst_addr)
{
    return (inst_addr >> instShiftAmt) & indexMask;
}


bool LVPUnit::predict(const DynInstPtr &inst)
{
    
    ThreadID tid = inst->threadNumber;
    const PCStateBase &pc = inst->pcState();

    unsigned idx= getIndex(pc.instAddr());
    uint8_t counter_val = lvp_table[idx].cntr;
    bool is_predictible_ld = false;

    if(counter_val > 1)
    {
        is_predictible_ld = true;
        stats.LCTPredictable +=1;
    }

    stats.LCTLookups += 1;
    
    if (is_predictible_ld == false) 
    {
        inst -> setLdPredictible(false);
        inst -> setLdConstant(false);
        inst -> PredictedLdValue(lvp_table[idx].data_value);

        DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu]  PC:0x%x (idx:%u) Not Predictible \n", inst->threadNumber, inst->seqNum, pc.instAddr(), idx);
        return false;
    }
    else
    {   
        ++stats.LVPTLookups;

        if (lvp_table[idx].valid)
        {
            ++stats.LVPTHits;
            ++stats.ldvalPredicted;

            uint64_t ld_predict_val = lvp_table[idx].data_value;
            inst->PredictedLdValue(ld_predict_val);
            inst -> setLdConstant(counter_val == 3);
            inst -> setLdPredictible(true);
    
            DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] PC:0x%x (idx:%u) ld_val:%llu LVP predicted predictible\n", inst->threadNumber, inst->seqNum,  pc.instAddr(), ld_predict_val, idx);
            return true;
        }
        else       
        {
            inst -> PredictedLdValue(0);
            inst -> setLdConstant(false);
            inst -> setLdPredictible(false);

            DPRINTF(LVPUnit, "lvpt_pred: [tid:%i] [sn:%llu] PC:0x%x (idx:%u) **** LVPT and LCT outcome not matching **** \n", tid, inst->seqNum, pc.instAddr(), idx);
            return false;
        }

        return true;
    }
}

void LVPUnit::update(const DynInstPtr &inst, uint64_t mem_ld_value)
{
    const PCStateBase &pc = inst->pcState();
    ThreadID tid = inst->threadNumber;
    unsigned idx= getIndex(pc.instAddr());


    bool valid_entry = lvp_table[idx].valid;

    if (!valid_entry)
    {
        DPRINTF(LVPUnit, "lvp_fresh_add: [tid:%i] [sn:%llu] PC:0x%x (idx:%u) data_addr:%llu ld_val:%u \n",
            inst->threadNumber, inst->seqNum, pc.instAddr(), idx, inst->effAddr, mem_ld_value);

        lvp_table[idx].data_value = mem_ld_value;
        lvp_table[idx].data_addr = inst->effAddr;
        lvp_table[idx].valid = true;
        lvp_table[idx].cntr = 1;
    }
    else
    {       
        uint64_t lvpt_ld_value = lvp_table[idx].data_value;
        uint64_t pred_ld_value = inst->PredictedLdValue();

        DPRINTF(LVPUnit, "lvp_update: [tid:%i] [sn:%llu] PC:0x%x (idx: %u) lvpt_ld_value:%llu pred_ld_value:%llu  mem_ld_value=%llu LVPT ENTRY == PREDICTED VALUE\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), idx, lvpt_ld_value, pred_ld_value, mem_ld_value);
    
        if (pred_ld_value == mem_ld_value)
        {
            // make the counter to predictible
            if(lvp_table[idx].cntr < 3)
            {
                lvp_table[idx].cntr = lvp_table[idx].cntr+1;
            }  

            lvp_table[idx].data_value = mem_ld_value;
            lvp_table[idx].data_addr = inst->effAddr;
            lvp_table[idx].valid = true;
        }
        else
        {
            // make the counter to not predictible
            if(lvp_table[idx].cntr > 0)
            {
                lvp_table[idx].cntr = lvp_table[idx].cntr-1;
            }

            lvp_table[idx].data_value = mem_ld_value;
            lvp_table[idx].data_addr = inst->effAddr;
            lvp_table[idx].valid = true;

            stats.ldvalIncorrect++;
        }
         
    }
}

void LVPUnit::cvu_invalidate(const DynInstPtr &inst) {

    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr StdataAddr = inst->effAddr;
    ThreadID tid = inst->threadNumber;
    unsigned idx= getIndex(pc.instAddr());

     for (unsigned i = 0; i < numEntries; ++i) 
     {   
        if(lvp_table[i].valid == true && lvp_table[i].data_addr == StdataAddr)
        {
            lvp_table[i].valid = false;
            lvp_table[i].cntr = 0;
            lvp_table[i].data_addr = 0;
            lvp_table[i].data_value = 0;

            DPRINTF(LVPUnit, "cvu_inv: [tid:%i] [sn:%llu] PC:0x%x (idx: %u) data_addr:%llu\n",
                    inst->threadNumber, inst->seqNum, pc.instAddr(), idx, StdataAddr);
    

        }
    }

    return;
}

bool LVPUnit::cvu_valid(const DynInstPtr &inst) {

    const PCStateBase &pc = inst->pcState();
    Addr instPC = pc.instAddr();
    Addr LwdataAddr = inst-> effAddr;
    ThreadID tid = inst->threadNumber;

    unsigned idx= getIndex(pc.instAddr());

    
    if(lvp_table[idx].valid && lvp_table[idx].data_addr == LwdataAddr && lvp_table[idx].cntr == 3)
    {
        DPRINTF(LVPUnit, "cvu_valid: [tid:%i] [sn:%llu] PC:0x%x (idx: %u) data_addr:%llu lvpt.data_addr:%llu\n",
                inst->threadNumber, inst->seqNum, pc.instAddr(), idx, LwdataAddr, lvp_table[idx].data_addr);
        return true;
    }
    else
    {
        return false;
    }
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

 LVPUnit* LVPUnit::create(CPU *_cpu, const BaseO3CPUParams &params)
 {
    return new LVPUnit(_cpu, params);
 }

} // namespace o3
} // namespace gem5