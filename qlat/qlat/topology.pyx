# cython: binding=True, embedsignature=True, c_string_type=unicode, c_string_encoding=utf8

from qlat_utils.all cimport *
from . cimport everything as cc
from .qcd cimport GaugeField
from .field_types cimport FieldRealD
from .gauge_action cimport GaugeAction
from .hmc cimport GaugeMomentum

from .hmc import set_gm_force, gf_evolve
from .wilson_flow import gf_wilson_flow_step, gf_energy_density
import qlat_utils as q

from pprint import pformat

@q.timer
def gf_topology_field_clf(GaugeField gf):
    """
    return topf
    topf.geo().multiplicity == 1
    Use the basic gf_clover_leaf_field
    NOT using 5 loop improved definition
    """
    topf = FieldRealD()
    cc.clf_topology_field(topf.xx, gf.xxx().val())
    return topf

@q.timer
def gf_topology_clf(GaugeField gf):
    """
    return top
    ininstance(top, float)
    Use the basic gf_clover_leaf_field
    NOT using 5 loop improved definition
    """
    return gf_topology_field_clf(gf).glb_sum()[:].item()

@q.timer
def gf_topology_field(GaugeField gf):
    """
    return topf
    topf.geo().multiplicity == 1
    Using the 5 loop improved definition
    https://arxiv.org/pdf/hep-lat/9701012v2.pdf
    """
    topf = FieldRealD()
    cc.clf_topology_field_5(topf.xx, gf.xxx().val())
    return topf

@q.timer
def gf_topology(GaugeField gf):
    """
    return top
    ininstance(top, float)
    Using the 5 loop improved definition
    https://arxiv.org/pdf/hep-lat/9701012v2.pdf
    """
    return gf_topology_field(gf).glb_sum()[:].item()

@q.timer
def gf_topology_terms_field(GaugeField gf):
    """
    return topf;
    topf.geo().multiplicity() == 5
    sum of the 5 terms should equal to gf_topology_field
    """
    topf = FieldRealD()
    cc.clf_topology_field_5_terms(topf.xx, gf.xxx().val())
    return topf

@q.timer
def gf_topology_terms(GaugeField gf):
    """
    return top_terms;
    top_terms.shape == (5,)
    top_terms.dtype == np.float64
    sum of the 5 terms should equal to gf_topology
    """
    return gf_topology_terms_field(gf).glb_sum()[0, :]

@q.timer_verbose
def smear_measure_topo(gf, smear_info_list=None, *, is_show_topo_terms=False):
    """
    smear_info = [ [ step_size, n_step, c1 = 0.0, wilson_flow_integrator_type = "runge-kutta", ], ... ]
    c1 = 0.0 # Wilson
    c1 = -0.331 # Iwasaki
    c1 = -1.4008 # DBW2
    wilson_flow_integrator_type = "runge-kutta"
    wilson_flow_integrator_type = "euler"
    """
    if smear_info_list is None:
        smear_info_list = [
                [ 0.05, 20, 0.0, "euler", ],
                [ 0.05, 20, 0.0, "euler", ],
                [ 0.05, 20, 0.0, "euler", ],
                [ 0.01, 50, -1.4008, "euler", ],
                [ 0.01, 50, -1.4008, "euler", ],
                [ 0.01, 50, -1.4008, "euler", ],
                [ 0.01, 50, -1.4008, "euler", ],
                ]
    q.displayln_info(f"smear_info_list =")
    q.displayln_info(pformat(smear_info_list))
    flow_time = 0
    topo_list = []
    @q.timer
    def smear(step_size, n_step, c1=0.0, wilson_flow_integrator_type="runge-kutta"):
        nonlocal flow_time
        flow_time += n_step * step_size
        for i in range(n_step):
            gf_wilson_flow_step(gf, step_size, c1=c1, wilson_flow_integrator_type=wilson_flow_integrator_type)
    @q.timer
    def measure():
        gf.show_info()
        plaq = gf.plaq()
        energy_density = gf_energy_density(gf)
        topo_field_clf = gf_topology_field_clf(gf)
        topo_clf = topo_field_clf.glb_sum()[:, :].item()
        t_sum_clf = topo_field_clf.glb_sum_tslice()
        t_sum_clf = [ t_sum_clf.get_elem(t).item() for t in range(t_sum_clf.n_points()) ]
        topo_field = gf_topology_field(gf)
        topo = topo_field.glb_sum()[:, :].item()
        t_sum = topo_field.glb_sum_tslice()
        t_sum = [ t_sum.get_elem(t).item() for t in range(t_sum.n_points()) ]
        q.displayln_info(f"t={flow_time} topo_clf={topo_clf} topo={topo}")
        q.displayln_info(pformat(list(enumerate(zip(t_sum_clf, t_sum)))))
        topo_list.append({
            "flow_time": flow_time,
            "plaq": plaq,
            "energy_density": energy_density,
            "topo_clf": topo_clf,
            "topo_clf_tslice": t_sum_clf,
            "topo": topo,
            "topo_tslice": t_sum,
            })
        if is_show_topo_terms:
            topo_terms = gf_topology_terms(gf)
            q.displayln_info(f"t={flow_time} topo={topo} {sum(topo_terms)}")
            topo_terms_str = ',\n '.join([ str(x) for x in topo_terms ])
            q.displayln_info(f"[ {topo_terms_str},\n]")
    measure()
    for si in smear_info_list:
        smear(*si)
        measure()
    return topo_list
