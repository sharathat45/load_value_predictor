#ifndef __CPU_LVPT_HH__
#define __CPU_LVPT_HH__

#include <vector>

#include "base/logging.hh"
#include "base/types.hh"

namespace gem5
{

namespace o3
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
    LVPT(unsigned _numEntries, unsigned _shiftAmt, unsigned _numThreads);

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

} // namespace o3
} // namespace gem5

#endif // __CPU_LVPT_HH__
