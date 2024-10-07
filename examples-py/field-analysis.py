#!/usr/bin/env python3

json_results = []
check_eps = 1e-10

import sys
import qlat as q
import numpy as np

size_node_list = [
    [1, 1, 1, 1],
    [1, 1, 1, 2],
    [1, 1, 1, 4],
    [1, 1, 1, 8],
    [2, 2, 2, 2],
    [2, 2, 2, 4]]

q.begin_with_mpi(size_node_list)

q.qremove_all_info("results")
q.qmkdir_info("results")

total_site = q.Coordinate([ 4, 4, 4, 8, ])
geo = q.Geometry(total_site)
q.displayln_info("CHECK: geo.show() =", geo.show())
rs = q.RngState("seed")

f1 = q.FieldComplexD(geo, 3)
f2 = q.FieldComplexD(geo, 5)
f1.set_rand(rs.split("f1"))
f2.set_rand(rs.split("f2"))

json_results.append(("f1 data sig", q.get_data_sig(f1, rs), check_eps,))
json_results.append(("f2 data sig", q.get_data_sig(f2, rs), check_eps,))

radius = 2.0
sf1 = q.smear_field(f1, radius)
sf2 = q.smear_field(f2, radius, is_only_spatial=True)

json_results.append(("sf1 data sig", q.get_data_sig(sf1, rs), check_eps,))
json_results.append(("sf2 data sig", q.get_data_sig(sf2, rs), check_eps,))

idx1 = np.array([ 0, 1, ], dtype=np.int32)
idx2 = np.array([ 0, 2, ], dtype=np.int32)

ff = q.field_convolution(f1, f2, idx1, idx2)

json_results.append(("ff data sig", q.get_data_sig(ff, rs), check_eps,))

ff3d = q.field_convolution(f1, f2, idx1, idx2, is_only_spatial=True)

json_results.append(("ff3d data sig", q.get_data_sig(ff3d, rs), check_eps,))

q.check_log_json(__file__, json_results)

q.timer_display()

q.end_with_mpi()

q.displayln_info(f"CHECK: finished successfully.")
