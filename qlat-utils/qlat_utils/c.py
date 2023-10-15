# ----------------------------------

__all__ = []

__all__ += [
        'CoordinateD',
        'Coordinate',
        'mod',
        'smod',
        'middle_mod',
        'coordinate_from_index',
        'index_from_coordinate',
        ]

__all__ += [
        'RngState',
        ]

__all__ += [
        'as_wilson_matrix',
        'as_wilson_matrix_g5_herm',
        'benchmark_matrix_functions',
        'WilsonMatrix',
        'SpinMatrix',
        'ColorMatrix',
        'get_gamma_matrix',
        'wilson_matrix_g5_herm',
        'mat_tr_sm',
        'mat_tr_cm',
        'mat_tr_wm',
        'mat_tr_wm_wm',
        'mat_tr_wm_sm',
        'mat_tr_sm_wm',
        'mat_tr_sm_sm',
        'mat_tr_wm_cm',
        'mat_tr_cm_wm',
        'mat_tr_cm_cm',
        'mat_mul_wm_wm',
        'mat_mul_wm_sm',
        'mat_mul_sm_wm',
        'mat_mul_sm_sm',
        'mat_mul_wm_cm',
        'mat_mul_cm_wm',
        'mat_mul_cm_cm',
        ]

__all__ += [
        'ElemType',
        'ElemTypeColorMatrix',
        'ElemTypeWilsonMatrix',
        'ElemTypeNonRelWilsonMatrix',
        'ElemTypeIsospinMatrix',
        'ElemTypeSpinMatrix',
        'ElemTypeWilsonVector',
        'ElemTypeComplex',
        'ElemTypeComplexF',
        'ElemTypeDouble',
        'ElemTypeFloat',
        'ElemTypeLong',
        'ElemTypeInt64t',
        'ElemTypeInt8t',
        'ElemTypeChar',
        ]

__all__ += [
        'flush',
        'get_id_node',
        'get_num_node',
        'verbose_level',
        'get_time',
        'get_start_time',
        'get_actual_start_time',
        'get_total_time',
        'get_actual_total_time',
        'timer_display',
        'timer_autodisplay',
        'timer_display_stack',
        'timer_display_stack_always',
        'timer_reset',
        'timer_fork',
        'timer_merge',
        'timer',
        'timer_verbose',
        'timer_flops',
        'timer_verbose_flops',
        'Timer',
        'TimerNone',
        'displayln',
        'displayln_info',
        ]

# ----------------------------------

import ctypes
import sys
import os
flags = sys.getdlopenflags()
sys.setdlopenflags(flags | os.RTLD_GLOBAL)

lib_path = os.path.join(os.path.dirname(__file__),
                        'lib/libqlat-utils.so')

if not os.path.isfile(lib_path):
    lib_path = os.path.join(os.path.dirname(__file__),
                            'lib/libqlat-utils.dylib')

assert os.path.isfile(lib_path)

ctypes.CDLL(lib_path, mode=ctypes.RTLD_GLOBAL)

from .cpa import *
from .cp import *
from .timer import *

sys.setdlopenflags(flags)
