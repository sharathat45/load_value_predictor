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
     *  @param seq_num The sequence number of the instruction to lookup.
     *  @param tid The thread id.
     *  @return Returns 1 if the seq_num was found, 0 otherwise.
     */
    bool lookup(InstSeqNum seq_num, ThreadID tid, VPTTEntry& entry);

    /** Inserts a predicted load instruction into the VPTT.
     *  @param seq_num The sequence number of the instruction to insert.
     *  @param loadAddr The address of the load being inserted.
     *  @param tid The thread id.
     */
    void insert(InstSeqNum seq_num, Addr loadAddr, ThreadID tid);

    /** Deletes from the VPTT the entry for a load.
     *  @param seq_num The sequence number of the instruction to delete.
     *  @param tid The thread id.
     */
    void remove(InstSeqNum seq_num, ThreadID tid);

  private:
    /** The actual VPTT. */
    std::vector<VPTTEntry> vptt;

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
