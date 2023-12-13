from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *

class ValuePredictor(SimObject):
    type = 'ValuePredictor'
    cxx_class = 'gem5::VPredUnit'
    cxx_header = "cpu/vpred/vpred_unit.hh"
    abstract = True 

class LVP(ValuePredictor):
    type = 'LVP'
    cxx_class = 'gem5::LVP'
    cxx_header = "cpu/vpred/lvp.hh"

    lastPredictorSize = Param.Unsigned(8192, "Size of LVP predictor")
    lastCtrBits = Param.Unsigned(3, "Bits per counter")