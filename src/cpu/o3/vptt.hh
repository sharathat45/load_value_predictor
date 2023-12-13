#ifndef __CPU_VPTT_HH__
#define __CPU_VPTT_HH__

#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/inst_seq.hh"

namespace gem5
{

namespace o3
{

class VPTT
{
  private:
    struct VPTTEntry
    {
        /** The entry's instruction seq num. */
        InstSeqNum seq_num;

        /** The entry's data address. */
        Addr addr;

        /** The entry's load value. */
        uint64_t value;

        /** The entry's thread id. */
        ThreadID tid;

        /** Whether or not the entry is valid. */
        bool valid = false;
    };

  public:
    /** Creates a VPTT with the given number of entries, number of bits per
     *  tag, and instruction offset amount.
     *  @param numEntries Number of entries for the VPTT.
     *  @param shiftAmt Offset amount for load addresses to ignore alignment.
     *  @param numThreads Number of supported threads.
     */
    VPTT(unsigned _numEntries, unsigned _shiftAmt, unsigned _numThreads);

    void reset();

    /** Looks up an address in the VPTT. Must call valid() first on the address.
     *  @param loadAddr The address of the load to look up.
     *  @param tid The thread id.
     *  @return Returns the predicted load value.
     */
    uint64_t lookup(Addr loadAddr, ThreadID tid);

    /** Checks if a load is in the VPTT.
     *  @param loadAddr The address of the load to look up.
     *  @param tid The thread id.
     *  @return Whether or not the index is valid in the VPTT.
     */
    bool valid(Addr loadAddr, ThreadID tid);

    /** Updates the VPTT with the value of a load.
     *  @param loadAddr The address of the load being updated.
     *  @param loadValue The value of the load being updated.
     *  @param tid The thread id.
     */
    void insert(Addr loadAddr, uint64_t loadValue, ThreadID tid);

    /** Deletes from the VPTT the entry for a load.
     *  @param loadAddr The address of the load being updated.
     *  @param tid The thread id.
     */
    void remove(InstSeqNum seq_num, ThreadID tid);

  private:
    /** Returns the index into the VPTT, based on the load's address.
     *  @param loadAddr The load to look up.
     *  @return Returns the index into the VPTT.
     */
    inline unsigned getIndex(Addr loadAddr, ThreadID tid);

    /** The actual VPTT. */
    std::vector<VPTTEntry> VPTT;

    /** The number of entries in the VPTT. */
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

#endif // __CPU_VPTT_HH__
