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

#ifndef __CPU_PRED_BTB_HH__
#define __CPU_PRED_BTB_HH__

#include "arch/generic/pcstate.hh"
#include "base/logging.hh"
#include "base/types.hh"
#include "config/the_isa.hh"

namespace gem5
{

namespace o3
{

class CVU
{
  private:
    struct CVUEntry
    {
        /** The entry's tag. */
        unsigned instr_idx = 0;
        Addr data_addr = 0;
        
        /** The entry's thread id. */
        ThreadID tid;

        /* constant data */
        RegVal data;

        /** Whether or not the entry is valid. */
        bool valid = false;

        unsigned char reference;
    };

  public:
    /** Creates a Table with the given number of entries, number of bits per
     *  tag, and instruction offset amount.
     *  @param numEntries Number of entries for the CVU.
     *  @param tagBits Number of bits for each tag in the CVU.
     *  @param instShiftAmt Offset amount for instructions to ignore alignment.
     */
    CVU(unsigned numEntries, unsigned indexBits,
               unsigned instShiftAmt, unsigned numThreads);

    void reset();

    /** Looks up an address in the CVU table. Must call valid() first on the address.
     *  @param index Index returned from the valid function
     *  @return The value the load instruction should get
     */
    RegVal lookup(int index, ThreadID tid);

    /** Checks if a branch is in the CVU.
     *  @param inst_PC The address of the load to look up.
     *  @param LwdataAddr The address of the load data.
     *  @param tid The thread id.
     *  @return index of the entry if the entry found in 
     */
    int valid(Addr instPC, Addr LwdataAddr, ThreadID tid);

    /** Updates the CVU entry.
     *  @param instPc The address of the ld instruction being updated.
     *  @param data_addr The data address
     *  @param data The value the load instruction should load
     *  @param tid The thread id.
     */
    void update(Addr instPc, Addr data_addr, RegVal data, ThreadID tid);

    // invalidate the corresponding entry
    // return true if a matching entry was found
    bool invalidate(Addr instPC, Addr LwdataAddr, ThreadID tid);

    // replace the LRU entry, should be called if the table is full
    void replacement(unsigned instr_idx, Addr data_addr, RegVal data, ThreadID tid);

    // keep track of the LRU, should be called everytime
    // an entry is referenced
    void reference_update(int index);

  private:
    /** Returns the index into the BTB, based on the branch's PC.
     *  @param inst_PC The branch to look up.
     *  @return Returns the index into the BTB.
     */
    inline unsigned getIndex(Addr instPC, ThreadID tid);

    /** The actual CVU table. */
    std::vector<CVUEntry> cvu_table;

    /** The number of entries in the CVU Table. */

    unsigned numEntries;

    /** The index mask. */
    unsigned idxMask;

    /* set reference bit */
    unsigned char reference_bit = 0x80; // 0x1000 0000
    /* Number of bits for index used in the LVPT*/
    unsigned indexBits;

    /** Number of bits to shift PC when calculating index. */
    unsigned instShiftAmt;

    /** Log2 NumThreads used for hashing threadid */
    unsigned log2NumThreads;
};

} // namespace branch_prediction
} // namespace gem5

#endif // __CPU_PRED_BTB_HH__
