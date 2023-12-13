#ifndef __CPU_PRED_VPRED_UNIT_HH__
#define __CPU_PRED_VPRED_UNIT_HH__


#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/inst_seq.hh"
#include "cpu/static_inst.hh"
#include "sim/probe/pmu.hh"
#include "sim/sim_object.hh"
#include "sim/stats.hh"
#include "params/ValuePredictor.hh"

namespace gem5
{

class VPredUnit : public SimObject
{
    public:
        typedef ValuePredictorParams Params;

        VPredUnit(const Params &p);

        // void regStats() override;

        /*
        * Predicts whether to do VP and returns the predicted value by reference. 
        */
        bool predict(const StaticInstPtr &inst, Addr inst_addr, RegVal &value);

        virtual bool lookup(Addr inst_addr, RegVal &value) = 0;

        float getpredictconf(Addr inst_addr, RegVal &value);

        virtual float getconf(Addr inst_addr, RegVal &value) = 0;

        void update(const StaticInstPtr &inst, Addr inst_addr, bool isValuePredicted, bool isValueTaken, RegVal &trueValue);
        
        virtual void updateTable(Addr inst_addr, bool isValuePredicted, bool isValueTaken, RegVal &trueValue) = 0;

    private:
        struct VPredUnitStats : public statistics::Group
        {
            VPredUnitStats(statistics::Group *parent);

            /** Stat for number of lookups. */
            statistics::Scalar lookups;
            /** Stat for number of lds predicted. */
            statistics::Scalar predTotal;
            /** Stat for number of lds predicted correctly. */
            statistics::Scalar predCorrect;
            /** Stat for number of lds predicted incorrectly. */
            statistics::Scalar predIncorrect;
            /** Stat for fraction of predicted lds predicted correctly. */
            statistics::Formula predAccuracy;
        } stats;
};
} // namespace gem5
#endif // __CPU_PRED_VPRED_UNIT_HH__
