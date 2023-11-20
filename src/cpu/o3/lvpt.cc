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

#include "cpu/o3/lvpt.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/IEW.hh"

namespace gem5
{

namespace load_value_prediction
{

LVPT::LVPT(unsigned _numEntries,
           unsigned _shiftAmt,
           unsigned _numThreads)
    : numEntries(_numEntries),
      shiftAmt(_shiftAmt),
      log2NumThreads(floorLog2(_numThreads))
{
    DPRINTF(IEW, "LVPT: Creating LVPT object.\n");

    if (!isPowerOf2(numEntries)) {
        fatal("LVPT entries is not a power of 2!");
    }

    lvpt.resize(numEntries);

    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }

    idxMask = numEntries - 1;
}

void
LVPT::reset()
{
    for (unsigned i = 0; i < numEntries; ++i) {
        lvpt[i].valid = false;
    }
}

inline
unsigned
LVPT::getIndex(Addr loadAddr, ThreadID tid)
{
    // Need to shift load address over by the word offset.
    return (loadAddr >> shiftAmt) & idxMask;
}

bool
LVPT::valid(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid && lvpt[lvpt_idx].tid == tid) {
        return true;
    } else {
        return false;
    }
}

RegVal
LVPT::lookup(Addr loadAddr, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    if (lvpt[lvpt_idx].valid && lvpt[lvpt_idx].tid == tid) {
        return lvpt[lvpt_idx].value;
    } else {
        return 0xBAD1BAD1;
    }
}

void
LVPT::update(Addr loadAddr, RegVal loadValue, ThreadID tid)
{
    unsigned lvpt_idx = getIndex(loadAddr, tid);

    assert(lvpt_idx < numEntries);

    lvpt[lvpt_idx].tid = tid;
    lvpt[lvpt_idx].valid = true;
    lvpt[lvpt_idx].value = loadValue;
}

} // namespace load_value_prediction
} // namespace gem5
