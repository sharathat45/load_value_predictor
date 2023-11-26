/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/o3/cvu.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"

namespace gem5
{

namespace o3
{

CVU::CVU(unsigned _numEntries,
                       unsigned _indexBits,
                       unsigned _instShiftAmt,
                       unsigned _num_threads)
    : numEntries(_numEntries),
      indexBits(_indexBits),
      instShiftAmt(_instShiftAmt)
{
    DPRINTF(Fetch, "CVU: Creating CVU object.\n");

    
    cvu_table.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].valid = false;
    }

    idxMask = (1 << indexBits) - 1;
}

void
CVU::reset()
{
    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].valid = false;
    }
}

inline
unsigned
CVU::getIndex(Addr instPC, ThreadID tid)
{
    // Need to shift PC over by the word offset.
    return ((instPC >> instShiftAmt)
            ^ (tid << (tagShiftAmt - instShiftAmt - log2NumThreads)))
            & idxMask;
}

// LRU
void
CVU::reference_update(int index)
{
    for (unsigned i = 0; i < numEntries; ++i) {
        cvu_table[i].reference = cvu_table[i].reference >> 1;
        
        if (i == index) {
            cvu_table[i].reference = cvu_table[i].reference | reference_bit;
        }
    }
}

// If LCT Predict Constant, call this function to check if CVU
// already contained the corresponding entry
// return the index of the entry in the table if entry was found
// else return -1
int
CVU::valid(Addr instPC, Addr LwdataAddr, ThreadID tid)
{
    unsigned instr_idx = getIndex(instPC, tid);

    assert(instr_idx < numEntries);

    for (unsigned i = 0;i < numEntries; ++i){
        if (cvu_table[i].valid
            && LwdataAddr == cvu_table[i].data_addr,
            && instr_idx == cvu_table[i].instr_idx,
            && cvu_table[i].tid == tid){
                reference_update(i);
                return i;
            }

    }

    return -1;
}


// Return True if the corresponding entry appears in the table
// and has been invalidated. Return False if no corresponding
// entry was found.
bool
CVU::invalidate(Addr instPC, Addr LwdataAddr, ThreadID tid)
{   
    unsigned instr_idx = getIndex(instPC, tid);

    assert(instr_idx < numEntries);
    
    for (unsigned i = 0;i < numEntries; ++i){
        if (cvu_table[i].valid
            && LwdataAddr == cvu_table[i].data_addr,
            && instr_idx == cvu_table[i].instr_idx,
            && cvu_table[i].tid == tid){
                cvu_table[i].valid = false;
                cvu_table[i].data_addr = 0;
                cvu_table[i].data = 0;
                cvu_table[i].instr_idx = 0;
                cvu_table[i].reference = 0;
                cvu_table[i].tid = 0;
                return true;
            }
    }

    return false;
    
}

// should only be called after the valid function
// if the corresponding entry is valid
const RegVal
CVU::lookup(int index, ThreadID tid)
{   
    assert((index >= 0) && (index < numEntries));
    assert(cvu_table[index].valid);

    return cvu_table[index].data;
}

// 
void
CVU::replacement(unsigned instr_idx, Addr data_addr, RegVal data, ThreadID tid)
{
    unsigned char LRU = 0xff;
    unsigned LRU_idx = 0;
    RegVal old_data_addr;

    for (unsigned i = 0;i < numEntries; i++){
        if (cvu_table[i].reference < LRU) {
            LRU = cvu_table[i].reference;
            LRU_idx = i;
        }
    }

    old_data_addr = cvu_table[LRU_idx].data_addr;
    cvu_table[LRU_idx].valid = true;
    cvu_table[LRU_idx].data_addr = data_addr;
    cvu_table[LRU_idx].instr_idx = instr_idx;
    cvu_table[LRU_idx].data = data;
    cvu_table[LRU_idx].tid = tid;
    cvu_table[LRU_idx].reference = 0;
    reference_update(LRU_idx);

    //DPRINTF(LVPTunit, "CVU LRU replacement table entry [%d]: %d -> %d", LRU_idx, old_data_addr, data_addr);

    return LRU_idx;
}

void
CVU::update(Addr instPc, Addr data_addr, RegVal data, ThreadID tid)
{
    unsigned instr_idx = getIndex(instPc, tid);

    assert(instr_idx < numEntries);
    
    for (unsigned i = 0; i < numEntries; ++i) {
        if (cvu_table[i].valid == false) {
            cvu_table[i].instr_index = instr_idx;
            cvu_table[i].data_addr = data_addr;
            cvu_table[i].data = data;
            cvu_table[i].tid = tid;
            cvu_table[i].valid = true;
            reference_update(i);
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
