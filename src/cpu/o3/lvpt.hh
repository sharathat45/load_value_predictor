/*
 * Copyright (c) 2023 Purdue University
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

#ifndef __CPU_LVPT_HH__
#define __CPU_LVPT_HH__

#include <vector>

#include "base/logging.hh"
#include "base/types.hh"

namespace gem5
{

namespace load_value_prediction
{

class LVPT
{
  private:
    struct LVPTEntry
    {
        /** The entry's load value. */
        RegVal value;

        /** The entry's thread id. */
        ThreadID tid;

        /** Whether or not the entry is valid. */
        bool valid = false;
    };

  public:
    /** Creates a LVPT with the given number of entries, number of bits per
     *  tag, and instruction offset amount.
     *  @param numEntries Number of entries for the LVPT.
     *  @param shiftAmt Offset amount for load addresses to ignore alignment.
     *  @param numThreads Number of supported threads.
     */
    LVPT(unsigned numEntries, unsigned shiftAmt, unsigned numThreads);

    void reset();

    /** Looks up an address in the LVPT. Must call valid() first on the address.
     *  @param loadAddr The address of the load to look up.
     *  @param tid The thread id.
     *  @return Returns the predicted load value.
     */
    RegVal lookup(Addr loadAddr, ThreadID tid);

    /** Checks if a load is in the LVPT.
     *  @param loadAddr The address of the load to look up.
     *  @param tid The thread id.
     *  @return Whether or not the index is valid in the LVPT.
     */
    bool valid(Addr loadAddr, ThreadID tid);

    /** Updates the LVPT with the value of a load.
     *  @param loadAddr The address of the load being updated.
     *  @param loadValue The value of the load being updated.
     *  @param tid The thread id.
     */
    void update(Addr loadAddr, RegVal loadValue, ThreadID tid);

  private:
    /** Returns the index into the LVPT, based on the load's address.
     *  @param loadAddr The load to look up.
     *  @return Returns the index into the LVPT.
     */
    inline unsigned getIndex(Addr loadAddr, ThreadID tid);

    /** The actual LVPT. */
    std::vector<LVPTEntry> lvpt;

    /** The number of entries in the LVPT. */
    unsigned numEntries;

    /** The index mask. */
    unsigned idxMask;

    /** Number of bits to shift address when calculating index. */
    unsigned shiftAmt;

    /** Log2 NumThreads used for hashing threadid */
    unsigned log2NumThreads;
};

} // namespace load_value_prediction
} // namespace gem5

#endif // __CPU_LVPT_HH__
