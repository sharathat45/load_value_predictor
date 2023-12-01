#include "cpu/o3/cvu.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/LVPUnit.hh"

namespace gem5
{

namespace o3
{

CVU::CVU(unsigned _numEntries, unsigned _lvptnumentries, unsigned _instShiftAmt, unsigned _num_threads)
    : numEntries(_numEntries),
      LVPTnumEntries(_lvptnumentries),
      instShiftAmt(_instShiftAmt)
{
    DPRINTF(LVPUnit, "CVU: Creating CVU object.\n");
    
    cvu_table.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].valid = false;
    }

    idxMask = LVPTnumEntries - 1;
}

void CVU::reset()
{
    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].valid = false;
    }
}

inline unsigned CVU::getIndex(Addr instPC, ThreadID tid)
{
    // Need to shift PC over by the word offset.
    return ((instPC >> instShiftAmt) & idxMask);
            //^ (tid << (tagShiftAmt - instShiftAmt - log2NumThreads)))
            
}

// LRU
inline void CVU::LRU_update(int index)
{
    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].LRU = cvu_table[i].LRU >> 1;
        
        if (i == index) {
            cvu_table[i].LRU = cvu_table[i].LRU | LRU_bit;
        }
    }
}

// If LCT Predict Constant, call this function to check if CVU
// already contained the corresponding entry
// return the True if present, false if not
bool CVU::valid(Addr instPC, Addr LwdataAddr, ThreadID tid)
{
    unsigned instr_idx = getIndex(instPC, tid);

    assert(instr_idx < numEntries);

    for (unsigned i = 0;i < numEntries; ++i){
        if (cvu_table[i].valid
            && LwdataAddr == cvu_table[i].data_addr
            && instr_idx == cvu_table[i].instr_idx
            && cvu_table[i].tid == tid){
                DPRINTF(LVPUnit, "Valid Entry found in CVU[%d]",i);
                LRU_update(i);
                return true;
            }

    }

    return false;
}


// Return True if the corresponding entry appears in the table
// and has been invalidated. Return False if no corresponding
// entry was found.
bool CVU::invalidate(Addr instPC, Addr LwdataAddr, ThreadID tid)
{   
    unsigned instr_idx = getIndex(instPC, tid);

    assert(instr_idx < numEntries);
    
    for (unsigned i = 0;i < numEntries; ++i){
        if (cvu_table[i].valid
            && LwdataAddr == cvu_table[i].data_addr) {
                cvu_table[i].valid = false;
                cvu_table[i].data_addr = 0;
                cvu_table[i].instr_idx = 0;
                cvu_table[i].LRU = 0;
                cvu_table[i].tid = 0;
                return true;
            }
    }

    return false;
    
}

// 
inline void CVU::replacement(unsigned instr_idx, Addr data_addr, RegVal data, ThreadID tid)
{
    unsigned char LRU = 0xff;
    unsigned LRU_idx = 0;
    RegVal old_data_addr;
    
    // More efficient way?
    for (unsigned i = 0;i < numEntries; i++){
        if (cvu_table[i].LRU < LRU) {
            LRU = cvu_table[i].LRU;
            LRU_idx = i;
        }
    }

    old_data_addr = cvu_table[LRU_idx].data_addr;
    cvu_table[LRU_idx].valid = true;
    cvu_table[LRU_idx].data_addr = data_addr;
    cvu_table[LRU_idx].instr_idx = instr_idx;
    cvu_table[LRU_idx].tid = tid;
    cvu_table[LRU_idx].LRU = 0;
    LRU_update(LRU_idx);

    //DPRINTF(LVPUnit, "CVU LRU replacement table entry [%d]: %d -> %d", LRU_idx, old_data_addr, data_addr);

    return;
}

void CVU::update(Addr instPc, Addr data_addr, RegVal data, ThreadID tid)
{
    unsigned instr_idx = getIndex(instPc, tid);

    assert(instr_idx < numEntries);
    
    for (unsigned i = 0; i < numEntries; ++i) {
        if (cvu_table[i].valid == false) {
            cvu_table[i].instr_idx = instr_idx;
            cvu_table[i].data_addr = data_addr;
            cvu_table[i].tid = tid;
            cvu_table[i].valid = true;
            LRU_update(i);
            return;
        }
    }

    // if table is full 
    // Replace the least recently referenced entry
    replacement(instr_idx, data_addr, data, tid);

    return;
}

} // namespace branch_prediction
} // namespace gem5
