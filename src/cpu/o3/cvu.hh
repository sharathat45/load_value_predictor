#ifndef __CPU_CVU_HH__
#define __CPU_CVU_HH__

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
        unsigned instr_idx;
        Addr data_addr;
        
        /** The entry's thread id. */
        ThreadID tid;

        /* constant data */
        uint64_t data;

        /** Whether or not the entry is valid. */
        bool valid;

        unsigned char LRU;
    };

  public:
    /** Creates a Table with the given number of entries, number of bits per
     *  tag, and instruction offset amount.
     *  @param numEntries Number of entries for the CVU.
     *  @param tagBits Number of bits for each tag in the CVU.
     *  @param instShiftAmt Offset amount for instructions to ignore alignment.
     */
    CVU(unsigned _numEntries, unsigned _lvptnumentries, unsigned _instShiftAmt, unsigned _num_threads);
    
    void reset();

    /** Checks if a LVPT entry is in the CVU.
     *  @param inst_PC The address of the load to look up.
     *  @param LwdataAddr The address of the load data.
     *  @param tid The thread id.
     *  @return index of the entry if the entry found in 
     */
    bool valid(Addr instPC, Addr LwdataAddr, ThreadID tid);

    /** Updates the CVU entry.
     *  @param instPc The address of the ld instruction being updated.
     *  @param data_addr The data address
     *  @param data The value the load instruction should load
     *  @param tid The thread id.
     */
    void update(Addr instPc, Addr data_addr, uint64_t data, ThreadID tid);

    // invalidate the corresponding entry, call upon a store instruction
    // return true if a matching entry was found
    bool invalidate(Addr instPC, Addr LwdataAddr, ThreadID tid);

    // replace the LRU entry, should be called if the table is full
    void replacement(unsigned instr_idx, Addr data_addr, uint64_t data, ThreadID tid);

    // keep track of the LRU, should be called everytime
    // an entry is referenced
    void LRU_update(int index);

  private:
    /** Returns the index into the BTB, based on the branch's PC.
     *  @param inst_PC The branch to look up.
     *  @return Returns the index into the BTB.
     */
    inline unsigned getIndex(Addr instPC, ThreadID tid);

    /** The actual CVU table. */
    std::vector<CVUEntry> cvu_table;
    //CVUEntry cvu_table [256];

    /** The number of entries in the CVU Table. */

    unsigned numEntries;

    /** The index mask. */
    unsigned idxMask;

    /* set LRU bit */
    unsigned char LRU_bit = 0x80; // 0x1000 0000
    /* Number of bits for index used in the LVPT*/
    unsigned LVPTnumEntries;

    /** Number of bits to shift PC when calculating index. */
    unsigned instShiftAmt;

    /** Log2 NumThreads used for hashing threadid */
    unsigned log2NumThreads;
};

} // namespace branch_prediction
} // namespace gem5

#endif // __CPU_CVU_HH__
