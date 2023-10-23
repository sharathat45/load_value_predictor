/*
 * Copyright (c) 2013-2014 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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

#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/minor/decode.hh"
#include "cpu/minor/pipeline.hh"
#include "debug/Decode.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(Minor, minor);
namespace minor
{

Execute0::Execute0(const std::string &name_,
    MinorCPU &cpu_,
    const BaseMinorCPUParams &params,
    Latch<ForwardInstData>::Output inp_,
    Latch<ForwardInstData>::Input out_,
    std::vector<InputBuffer<ForwardInstData>> &next_stage_input_buffer) :
    Named(name_),
    cpu(cpu_),
    inp(inp_),
    out(out_),
    nextStageReserve(next_stage_input_buffer),
    outputWidth(params.executeInputWidth),
    processMoreThanOneInput(params.executeCycleInput),
    execute0Info(params.numThreads,
        ExecuteThreadInfo(params.executeCommitLimit)),
    issuePriority(0)
{
    if (outputWidth < 1)
    {
        fatal("%s: executeInputWidth must be >= 1 (%d)\n", name_, outputWidth);
    }

    if (params.execute0InputBufferSize < 1)
    {
        fatal("%s: execute0InputBufferSize must be >= 1 (%d)\n", name_,
              params.execute0InputBufferSize);
    }

    /* Per-thread structures */
    for (ThreadID tid = 0; tid < params.numThreads; tid++)
    {
        std::string tid_str = std::to_string(tid);

        /* Input Buffers */
        inputBuffer.push_back(
            InputBuffer<ForwardInstData>(
                name_ + ".inputBuffer" + tid_str, "insts",
                params.execute0InputBufferSize));
    }
}

const ForwardInstData *
Execute0::getInput(ThreadID tid)
{
    /* Get a line from the inputBuffer to work with */
    if (!inputBuffer[tid].empty())
    {
        const ForwardInstData &head = inputBuffer[tid].front();

        return (head.isBubble() ? NULL : &(inputBuffer[tid].front()));
    }
    else
    {
        return NULL;
    }
}

void Execute0::popInput(ThreadID tid)
{
    if (!inputBuffer[tid].empty())
        inputBuffer[tid].pop();

    execute0Info[tid].inputIndex = 0;
}

inline ThreadID
Execute0::getIssuingThread()
{
    std::vector<ThreadID> priority_list;

    switch (cpu.threadPolicy)
    {
    case enums::SingleThreaded:
        return 0;
    case enums::RoundRobin:
        priority_list = cpu.roundRobinPriority(issuePriority);
        break;
    case enums::Random:
        priority_list = cpu.randomPriority();
        break;
    default:
        panic("Invalid thread scheduling policy.");
    }

    for (auto tid : priority_list)
    {
        if (getInput(tid) && !execute0Info[tid].blocked)
        {
            issuePriority = tid;
            return tid;
        }
    }

    return InvalidThreadID;
}

bool Execute0::isDrained()
{
    for (const auto &buffer : inputBuffer)
    {
        if (!buffer.empty())
            return false;
    }

    return (*inp.outputWire).isBubble();
}

void
Execute0::evaluate()
{
     /* Push input onto appropriate input buffer */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].setTail(*inp.outputWire);

    ForwardInstData &insts_out = *out.inputWire;

    assert(insts_out.isBubble());

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++)
    {
        execute0Info[tid].blocked = !nextStageReserve[tid].canReserve();
    }

    ThreadID tid = getIssuingThread();

    if (tid != InvalidThreadID)
    {
        const ForwardInstData *insts_in = getInput(tid);

        // Forward instructions to execute stage
        int numForwarded = 0;
        while (insts_in &&
        execute0Info[tid].inputIndex < insts_in->width() &&
        /*Still more input*/
        numForwarded < outputWidth /* Still more output to fill */)
        {
            MinorDynInstPtr inst =
            insts_in->insts[execute0Info[tid].inputIndex];

            if (inst->isBubble())
            {
                /* Skip */
                execute0Info[tid].inputIndex++;
            }
            else
            {
                execute0Info[tid].inputIndex++;

                /* Correctly size the output before writing */
                if (numForwarded == 0)
                    insts_out.resize(outputWidth);
                /* Push into output */
                insts_out.insts[numForwarded] = inst;
                numForwarded++;
            }

            /* Got to the end of a line */
            if (execute0Info[tid].inputIndex == insts_in->width())
            {
                popInput(tid);
                /* Set insts_in to null to force us to leave the surrounding
                 *  loop */
                insts_in = NULL;
                if (processMoreThanOneInput)
                {
                    insts_in = getInput(tid);
                }
            }
        }
    }

    /* If we generated output, reserve space for the result in the next stage
     *  and mark the stage as being active this cycle */
    if (!insts_out.isBubble())
    {
        /* Note activity of following buffer */
        cpu.activityRecorder->activity();
        insts_out.threadId = tid;
        nextStageReserve[tid].reserve();
    }

    /* If we still have input to process and somewhere to put it,
     *  mark stage as active */
    for (ThreadID i = 0; i < cpu.numThreads; i++)
    {
        if (getInput(i) && nextStageReserve[i].canReserve())
        {
            cpu.activityRecorder->activateStage(Pipeline::Execute0StageId);
            break;
        }
    }

    /* Make sure the input (if any left) is pushed */
    if (!inp.outputWire->isBubble())
    {
        inputBuffer[inp.outputWire->threadId].pushTail();
    }

}

} // namespace minor
} // namespace gem5
