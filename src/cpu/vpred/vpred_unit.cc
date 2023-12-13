#include "cpu/vpred/vpred_unit.hh"

namespace gem5
{

VPredUnit::VPredUnit(const Params &params) :
	SimObject (params),
	stats(this)
{ }

VPredUnit::VPredUnitStats::VPredUnitStats(statistics::Group *parent)
    : statistics::Group(parent, "vpred"),
	ADD_STAT(lookups, statistics::units::Count::get(),
			"Number of prediction unit lookups"),
	ADD_STAT(predTotal, statistics::units::Count::get(),
			"Number of values predicted in total"),
	ADD_STAT(predCorrect, statistics::units::Count::get(),
			"Number of values predicted correctly"),
	ADD_STAT(predIncorrect, statistics::units::Count::get(),
			"Number of values predicted incorrectly"),
	ADD_STAT(predAccuracy, statistics::units::Ratio::get(),
			"Fraction of predicted load values that were correctly predicted",
			predCorrect / predTotal)
{ }

bool
VPredUnit::predict(const StaticInstPtr &inst, Addr inst_addr, RegVal &value)
{
	++stats.lookups;

	bool prediction= lookup(inst_addr, value);

	stats.predTotal += prediction;

	return prediction;
}

float
VPredUnit::getpredictconf(Addr inst_addr, RegVal &value)
{


	return getconf(inst_addr, value); // Returns prediction confidence


}


void
VPredUnit::update(const StaticInstPtr &inst, Addr inst_addr, bool isValuePredicted, bool isValueTaken, RegVal &trueValue)
{
	updateTable(inst_addr, isValuePredicted, isValueTaken, trueValue);

	if (isValuePredicted) {
		if (isValueTaken) {
			++stats.predCorrect;
		} else {
			++stats.predIncorrect;
		}
	}

	// if (isValuePredicted)
	// {
	// 	if (isValueTaken)
	// 	{
	// 		++numCorrectPredicted;
	// 		if (inst->isLoad())
	// 			++numLoadCorrectPredicted;
	// 	}
	// 	else
	// 	{
	// 		++numIncorrectPredicted;
	// 	}
	// }
}

} // namespace gem5


