#pragma once

#include <qlat/field.h>

namespace qlat
{  //

PointsSelection mk_tslice_point_selection(const int t_size, const int t_dir = 3);

PointsSelection mk_tslice_point_selection(const Coordinate& total_site,
                                          const int t_dir = 3);

PointsSelection mk_random_point_selection(const Coordinate& total_site,
                                         const Long num, const RngState& rs,
                                         const Long pool_factor = 2);

void save_point_selection(const PointsSelection& psel, const std::string& path);

void save_point_selection_info(const PointsSelection& psel,
                               const std::string& path);

PointsSelection load_point_selection(const std::string& path);

PointsSelection load_point_selection_info(const std::string& path);

crc32_t crc32_par(const PointsSelection& psel);

// -----------------------

template <class M>
bool is_initialized(const SelectedPoints<M>& sp)
{
  return sp.initialized;
}

template <class M>
bool is_consistent(const SelectedPoints<M>& sp, const PointsSelection& psel)
{
  return sp.initialized and sp.n_points == (Long)psel.size();
}

template <class M>
SelectedPoints<M>& operator+=(SelectedPoints<M>& f, const SelectedPoints<M>& f1)
{
  TIMER("sel_points_operator+=");
  if (not f.initialized) {
    f = f1;
  } else {
    qassert(f1.initialized);
    qassert(f.multiplicity == f1.multiplicity);
    qassert(f.n_points == f1.n_points);
    qassert(f.points.size() == f1.points.size());
    qacc_for(k, f.points.size(), { f.points[k] += f1.points[k]; });
  }
  return f;
}

template <class M>
SelectedPoints<M>& operator-=(SelectedPoints<M>& f, const SelectedPoints<M>& f1)
{
  TIMER("sel_points_operator-=");
  if (not f.initialized) {
    f.init(f1.n_points, f1.multiplicity);
    set_zero(f);
    f -= f1;
  } else {
    qassert(f1.initialized);
    qassert(f.multiplicity == f1.multiplicity);
    qassert(f.n_points == f1.n_points);
    qassert(f.points.size() == f1.points.size());
    qacc_for(k, f.points.size(), { f.points[k] -= f1.points[k]; });
  }
  return f;
}

template <class M>
SelectedPoints<M>& operator*=(SelectedPoints<M>& f, const double factor)
{
  TIMER("sel_points_operator*=(F,D)");
  qassert(f.initialized);
  qacc_for(k, f.points.size(), { f.points[k] *= factor; });
  return f;
}

template <class M>
SelectedPoints<M>& operator*=(SelectedPoints<M>& f, const ComplexD factor)
{
  TIMER("sel_points_operator*=(F,C)");
  qassert(f.initialized);
  qacc_for(k, f.points.size(), { f.points[k] *= factor; });
  return f;
}

template <class M>
void only_keep_selected_points(Field<M>& f, const PointsSelection& psel)
{
  TIMER("only_keep_selected_points");
  const Geometry& geo = f.geo();
  qassert(geo.is_only_local);
  Field<M> f1;
  f1.init(geo);
  set_zero(f1);
  const Long n_points = psel.size();
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      const Vector<M> fv = f.get_elems_const(xl);
      Vector<M> f1v = f1.get_elems(xl);
      for (int m = 0; m < geo.multiplicity; ++m) {
        f1v[m] = fv[m];
      }
    }
  });
  qswap(f, f1);
}

template <class M>
RealD qnorm(const SelectedPoints<M>& sp)
{
  TIMER("qnorm(sp)");
  return qnorm(sp.points);
}

template <class M>
void qnorm_field(SelectedPoints<RealD>& sp, const SelectedPoints<M>& sp1)
{
  TIMER("qnorm_field(sp,sp1)");
  sp.init();
  sp.init(sp1.n_points, 1);
  sp.distributed = sp1.distributed;
  qacc_for(idx, sp.n_points,
           { sp.get_elem(idx) = qnorm(sp1.get_elems_const(idx)); });
}

void set_sqrt_field(SelectedPoints<RealD>& sp,
                    const SelectedPoints<RealD>& sp1);

// -------------------------------------------

template <class M>
void set_selected_points(SelectedPoints<M>& sp, const Field<M>& f,
                         const PointsSelection& psel)
{
  TIMER("set_selected_points(sp,f,psel)");
  const Geometry& geo = f.geo();
  qassert(geo.is_only_local);
  const Long n_points = psel.size();
  sp.init(psel, geo.multiplicity);
  set_zero(sp);  // has to set_zero for glb_sum_byte_vec
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      const Vector<M> fv = f.get_elems_const(xl);
      Vector<M> spv = sp.get_elems(idx);
      for (int m = 0; m < geo.multiplicity; ++m) {
        spv[m] = fv[m];
      }
    }
  });
  glb_sum_byte_vec(get_data(sp.points));
}

template <class M>
void set_selected_points(SelectedPoints<M>& sp, const Field<M>& f,
                         const PointsSelection& psel, const int m)
{
  TIMER("set_selected_points(sp,f,psel,m)");
  const Geometry& geo = f.geo();
  qassert(geo.is_only_local);
  const Long n_points = psel.size();
  sp.init(psel, 1);
  set_zero(sp);  // has to set_zero for glb_sum_byte_vec
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      const Vector<M> fv = f.get_elems_const(xl);
      sp.get_elem(idx) = fv[m];
    }
  });
  glb_sum_byte_vec(get_data(sp.points));
}

template <class M>
void set_selected_points(SelectedPoints<M>& sp, const SelectedPoints<M>& sp0,
                         const PointsSelection& psel,
                         const PointsSelection& psel0,
                         const bool is_keeping_data = true)
// Most efficient if psel and psel0 is the same.
// If not, more efficient if psel and psel0 has the same order.
{
  if (&sp == &sp0) {
    return;
  }
  TIMER("set_selected_points(sp,sp0,psel,psel0)");
  if (&psel == &psel0) {
    sp = sp0;
    return;
  }
  const Long n_points = psel.size();
  const Long n_points0 = psel0.size();
  const Int multiplicity = sp0.multiplicity;
  if (is_keeping_data) {
    sp.init_zero(psel, multiplicity);
  } else {
    sp.init(psel, multiplicity);
    set_zero(sp);
  }
  SelectedPoints<Long> sp_idx;
  sp_idx.init(psel, 1);
  Long idx_last = -1;
  qfor(idx, n_points, {
    const Coordinate& xg = psel[idx];
    Long idx0 = -1;
    for (Long i = 0; i < n_points0; ++i) {
      idx_last += 1;
      if (idx_last >= n_points0) {
        idx_last = idx_last % n_points0;
      }
      const Coordinate& xg0 = psel0[idx_last];
      if (xg0 == xg) {
        idx0 = idx_last;
        break;
      }
    }
    sp_idx.get_elem(idx) = idx0;
  });
  qacc_for(idx, n_points, {
    const Long idx0 = sp_idx.get_elem(idx);
    if (idx0 >= 0) {
      qassert(idx0 < n_points0);
      const Vector<M> spv0 = sp0.get_elems_const(idx0);
      Vector<M> spv = sp.get_elems(idx);
      for (int m = 0; m < multiplicity; ++m) {
        spv[m] = spv0[m];
      }
    }
  });
}

template <class M>
void set_field_selected(Field<M>& f, const SelectedPoints<M>& sp,
                        const Geometry& geo_, const PointsSelection& psel,
                        const bool is_keeping_data = true)
{
  TIMER("set_field_selected(f,sp,geo,psel)");
  const Geometry geo = geo_reform(geo_, sp.multiplicity, 0);
  qassert(geo.is_only_local);
  const Long n_points = sp.n_points;
  qassert(n_points == (Long)psel.size());
  if (is_keeping_data) {
    f.init_zero(geo);
  } else {
    f.init(geo);
    set_zero(f);
  }
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      const Vector<M> spv = sp.get_elems_const(idx);
      Vector<M> fv = f.get_elems(xl);
      for (int m = 0; m < geo.multiplicity; ++m) {
        fv[m] = spv[m];
      }
    }
  });
}

template <class M>
void set_field_selected(Field<M>& f, const SelectedPoints<M>& sp,
                        const Geometry& geo_, const PointsSelection& psel,
                        const int m, const bool is_keeping_data = true)
{
  TIMER("set_field_selected(f,sp,geo,psel,m)");
  if (is_keeping_data) {
    f.init_zero(geo_);
  } else {
    f.init(geo_);
    set_zero(f);
  }
  const Geometry& geo = f.geo();
  const Long n_points = sp.n_points;
  qassert(n_points == (Long)psel.size());
  qassert(sp.multiplicity == 1);
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      Vector<M> fv = f.get_elems(xl);
      fv[m] = sp.get_elem(idx);
    }
  });
}

// -------------------------------------------

template <class M>
void acc_field(Field<M>& f, const SelectedPoints<M>& sp, const Geometry& geo_,
               const PointsSelection& psel)
{
  TIMER("acc_field(f,sp,geo,psel)");
  const Geometry geo = geo_reform(geo_, sp.multiplicity, 0);
  qassert(geo.is_only_local);
  const Long n_points = sp.n_points;
  qassert(n_points == (Long)psel.size());
  f.init(geo);
  qacc_for(idx, n_points, {
    const Coordinate& xg = psel[idx];
    const Coordinate xl = geo.coordinate_l_from_g(xg);
    if (geo.is_local(xl)) {
      const Vector<M> spv = sp.get_elems_const(idx);
      Vector<M> fv = f.get_elems(xl);
      for (int m = 0; m < geo.multiplicity; ++m) {
        fv[m] += spv[m];
      }
    }
  });
}

template <class M>
void field_glb_sum(SelectedPoints<M>& sp, const Field<M>& f)
{
  TIMER("field_glb_sum(sp,f)");
  sp.init();
  const Geometry& geo = f.geo();
  const int multiplicity = geo.multiplicity;
  std::vector<M> vec = field_glb_sum(f);
  sp.init(1, multiplicity);
  sp.points = vec;
}

template <class M>
void field_glb_sum_tslice(SelectedPoints<M>& sp, const Field<M>& f,
                          const int t_dir = 3)
{
  TIMER("field_glb_sum_tslice(sp,f)");
  sp.init();
  const Geometry& geo = f.geo();
  const int t_size = geo.total_site()[t_dir];
  const int multiplicity = geo.multiplicity;
  std::vector<M> vec = field_glb_sum_tslice(f, t_dir);
  sp.init(t_size, multiplicity);
  sp.points = vec;
}

// -------------------------------------------

template <class M>
void lat_data_from_selected_points(LatData& ld, const SelectedPoints<M>& sp)
{
  TIMER("lat_data_from_selected_points(sp)");
  qassert(is_composed_of_real_d<M>());
  ld.init();
  ld.info.push_back(lat_dim_number("idx", 0, sp.n_points - 1));
  ld.info.push_back(lat_dim_number("m", 0, sp.multiplicity - 1));
  if (is_composed_of_complex_d<M>()) {
    const Long n_v = sizeof(M) / sizeof(ComplexD);
    qassert(n_v * (Long)sizeof(ComplexD) == (Long)sizeof(M));
    ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
    ld.info.push_back(lat_dim_re_im());
  } else if (is_composed_of_real_d<M>()) {
    const Long n_v = sizeof(M) / sizeof(RealD);
    qassert(n_v * (Long)sizeof(RealD) == (Long)sizeof(M));
    ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
  } else {
    qerr(fname + ssprintf(": get_type_name(M)=%s", get_type_name<M>().c_str()));
  }
  lat_data_alloc(ld);
  assign(get_data(ld.res), get_data(sp.points));
}

template <class M>
void selected_points_from_lat_data(SelectedPoints<M>& sp, const LatData& ld)
{
  TIMER("selected_points_from_lat_data(sp,ld)");
  qassert(is_composed_of_real_d<M>());
  if (is_composed_of_complex_d<M>()) {
    qassert(ld.info.size() == 4);
  } else if (is_composed_of_real_d<M>()) {
    qassert(ld.info.size() == 3);
  } else {
    qerr(fname + ssprintf(": get_type_name(M)=%s", get_type_name<M>().c_str()));
  }
  qassert(ld.info[0].name == "idx");
  qassert(ld.info[1].name == "m");
  qassert(ld.info[2].name == "v");
  const Long n_points = ld.info[0].size;
  const Long multiplicity = ld.info[1].size;
  const Long sizof_M_vs_sizeof_v = ld.info[2].size;
  if (is_composed_of_complex_d<M>()) {
    qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(ComplexD));
    qassert(ld.info[3].name == "re-im");
    qassert(ld.info[3].size == 2);
  } else {
    qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(RealD));
  }
  sp.init(n_points, multiplicity);
  assign(get_data(sp.points), get_data(ld.res));
}

template <class M>
void lat_data_from_selected_points(LatDataRealF& ld, const SelectedPoints<M>& sp)
{
  TIMER("lat_data_from_selected_points(sp)");
  qassert(is_composed_of_real_f<M>());
  ld.init();
  ld.info.push_back(lat_dim_number("idx", 0, sp.n_points - 1));
  ld.info.push_back(lat_dim_number("m", 0, sp.multiplicity - 1));
  if (is_composed_of_complex_f<M>()) {
    const Long n_v = sizeof(M) / sizeof(ComplexF);
    qassert(n_v * (Long)sizeof(ComplexF) == (Long)sizeof(M));
    ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
    ld.info.push_back(lat_dim_re_im());
  } else if (is_composed_of_real_f<M>()) {
    const Long n_v = sizeof(M) / sizeof(RealF);
    qassert(n_v * (Long)sizeof(RealF) == (Long)sizeof(M));
    ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
  } else {
    qerr(fname + ssprintf(": get_type_name(M)=%s", get_type_name<M>().c_str()));
  }
  lat_data_alloc(ld);
  assign(get_data(ld.res), get_data(sp.points));
}

template <class M>
void selected_points_from_lat_data(SelectedPoints<M>& sp, const LatDataRealF& ld)
{
  TIMER("selected_points_from_lat_data(sp,ld)");
  qassert(is_composed_of_real_f<M>());
  if (is_composed_of_complex_f<M>()) {
    qassert(ld.info.size() == 4);
  } else if (is_composed_of_real_f<M>()) {
    qassert(ld.info.size() == 3);
  } else {
    qerr(fname + ssprintf(": get_type_name(M)=%s", get_type_name<M>().c_str()));
  }
  qassert(ld.info[0].name == "idx");
  qassert(ld.info[1].name == "m");
  qassert(ld.info[2].name == "v");
  const Long n_points = ld.info[0].size;
  const Long multiplicity = ld.info[1].size;
  const Long sizof_M_vs_sizeof_v = ld.info[2].size;
  if (is_composed_of_complex_d<M>()) {
    qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(ComplexF));
    qassert(ld.info[3].name == "re-im");
    qassert(ld.info[3].size == 2);
  } else {
    qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(RealF));
  }
  sp.init(n_points, multiplicity);
  assign(get_data(sp.points), get_data(ld.res));
}

template <class M>
void lat_data_from_selected_points(LatDataLong& ld, const SelectedPoints<M>& sp)
{
  TIMER("lat_data_from_selected_points(sp)");
  qassert(is_composed_of_long<M>());
  ld.init();
  ld.info.push_back(lat_dim_number("idx", 0, sp.n_points - 1));
  ld.info.push_back(lat_dim_number("m", 0, sp.multiplicity - 1));
  const Long n_v = sizeof(M) / sizeof(Long);
  qassert(n_v * (Long)sizeof(Long) == (Long)sizeof(M));
  ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
  lat_data_alloc(ld);
  assign(get_data(ld.res), get_data(sp.points));
}

template <class M>
void selected_points_from_lat_data(SelectedPoints<M>& sp, const LatDataLong& ld)
{
  TIMER("selected_points_from_lat_data(sp,ld)");
  qassert(is_composed_of_long<M>());
  qassert(ld.info.size() == 3);
  qassert(ld.info[0].name == "idx");
  qassert(ld.info[1].name == "m");
  qassert(ld.info[2].name == "v");
  const Long n_points = ld.info[0].size;
  const Long multiplicity = ld.info[1].size;
  const Long sizof_M_vs_sizeof_v = ld.info[2].size;
  qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(Long));
  sp.init(n_points, multiplicity);
  assign(get_data(sp.points), get_data(ld.res));
}

template <class M>
void lat_data_from_selected_points(LatDataInt& ld, const SelectedPoints<M>& sp)
{
  TIMER("lat_data_from_selected_points(sp)");
  qassert(is_composed_of_int<M>());
  ld.init();
  ld.info.push_back(lat_dim_number("idx", 0, sp.n_points - 1));
  ld.info.push_back(lat_dim_number("m", 0, sp.multiplicity - 1));
  const Long n_v = sizeof(M) / sizeof(Int);
  qassert(n_v * (Long)sizeof(Int) == (Long)sizeof(M));
  ld.info.push_back(lat_dim_number("v", 0, n_v - 1));
  lat_data_alloc(ld);
  assign(get_data(ld.res), get_data(sp.points));
}

template <class M>
void selected_points_from_lat_data(SelectedPoints<M>& sp, const LatDataInt& ld)
{
  TIMER("selected_points_from_lat_data(sp,ld)");
  qassert(is_composed_of_int<M>());
  qassert(ld.info.size() == 3);
  qassert(ld.info[0].name == "idx");
  qassert(ld.info[1].name == "m");
  qassert(ld.info[2].name == "v");
  const Long n_points = ld.info[0].size;
  const Long multiplicity = ld.info[1].size;
  const Long sizof_M_vs_sizeof_v = ld.info[2].size;
  qassert(sizeof(M) == sizof_M_vs_sizeof_v * sizeof(Int));
  sp.init(n_points, multiplicity);
  assign(get_data(sp.points), get_data(ld.res));
}

// -------------------------------------------

template <class M>
void save_selected_points(const SelectedPoints<M>& sp, QFile& qfile)
{
  TIMER_VERBOSE("save_selected_points(sp,qfile)");
  qassert(not sp.distributed);
  if (get_id_node() == 0) {
    if (is_composed_of_real_d<M>()) {
      LatData ld;
      lat_data_from_selected_points(ld, sp);
      ld.save(qfile);
    } else if (is_composed_of_real_f<M>()) {
      LatDataRealF ld;
      lat_data_from_selected_points(ld, sp);
      ld.save(qfile);
    } else if (is_composed_of_long<M>()) {
      LatDataLong ld;
      lat_data_from_selected_points(ld, sp);
      ld.save(qfile);
    } else if (is_composed_of_int<M>()) {
      LatDataInt ld;
      lat_data_from_selected_points(ld, sp);
      ld.save(qfile);
    } else {
      qassert(false);
    }
  }
}

template <class M>
void load_selected_points(SelectedPoints<M>& sp, QFile& qfile)
{
  TIMER_VERBOSE("load_selected_points(sp,qfile)");
  Long n_points = 0;
  Long multiplicity = 0;
  if (get_id_node() == 0) {
    if (is_composed_of_real_d<M>()) {
      LatData ld;
      ld.load(qfile);
      selected_points_from_lat_data(sp, ld);
    } else if (is_composed_of_real_f<M>()) {
      LatDataRealF ld;
      ld.load(qfile);
      selected_points_from_lat_data(sp, ld);
    } else if (is_composed_of_long<M>()) {
      LatDataLong ld;
      ld.load(qfile);
      selected_points_from_lat_data(sp, ld);
    } else if (is_composed_of_int<M>()) {
      LatDataInt ld;
      ld.load(qfile);
      selected_points_from_lat_data(sp, ld);
    } else {
      qassert(false);
    }
    n_points = sp.n_points;
    multiplicity = sp.multiplicity;
  }
  bcast(get_data_one_elem(n_points));
  bcast(get_data_one_elem(multiplicity));
  if (get_id_node() != 0) {
    sp.init(n_points, multiplicity);
  }
  vector<M> buffer(sp.points.size());
  assign(get_data(buffer), get_data(sp.points));
  bcast(get_data(buffer));
  assign(get_data(sp.points), get_data(buffer));
}

template <class M>
void save_selected_points(const SelectedPoints<M>& sp, const std::string& fn)
{
  QFile qfile;
  if (get_id_node() == 0) {
    qfile = qfopen(fn + ".partial", "w");
  }
  save_selected_points(sp, qfile);
  qfclose(qfile);
  qrename_info(fn + ".partial", fn);
}

template <class M>
void load_selected_points(SelectedPoints<M>& sp, const std::string& fn)
{
  QFile qfile;
  if (get_id_node() == 0) {
    qfile = qfopen(fn, "r");
  }
  load_selected_points(sp, qfile);
  qfclose(qfile);
}

// --------------------

#ifdef QLAT_INSTANTIATE_SELECTED_POINTS
#define QLAT_EXTERN
#else
#define QLAT_EXTERN extern
#endif

#define QLAT_EXTERN_TEMPLATE(TYPENAME)                                    \
                                                                          \
  QLAT_EXTERN template SelectedPoints<TYPENAME>& operator+=<TYPENAME>(    \
      SelectedPoints<TYPENAME>& f, const SelectedPoints<TYPENAME>& f1);   \
                                                                          \
  QLAT_EXTERN template SelectedPoints<TYPENAME>& operator-=<TYPENAME>(    \
      SelectedPoints<TYPENAME>& f, const SelectedPoints<TYPENAME>& f1);   \
                                                                          \
  QLAT_EXTERN template SelectedPoints<TYPENAME>& operator*=               \
      <TYPENAME>(SelectedPoints<TYPENAME>& f, const double factor);       \
                                                                          \
  QLAT_EXTERN template SelectedPoints<TYPENAME>& operator*=               \
      <TYPENAME>(SelectedPoints<TYPENAME>& f, const ComplexD factor);     \
                                                                          \
  QLAT_EXTERN template void only_keep_selected_points<TYPENAME>(          \
      Field<TYPENAME> & f, const PointsSelection& psel);                  \
                                                                          \
  QLAT_EXTERN template double qnorm<TYPENAME>(                            \
      const SelectedPoints<TYPENAME>& sp);                                \
                                                                          \
  QLAT_EXTERN template void qnorm_field<TYPENAME>(                        \
      SelectedPoints<double> & sp, const SelectedPoints<TYPENAME>& sp1);  \
                                                                          \
  QLAT_EXTERN template void set_selected_points<TYPENAME>(                \
      SelectedPoints<TYPENAME> & sp, const Field<TYPENAME>& f,            \
      const PointsSelection& psel);                                       \
                                                                          \
  QLAT_EXTERN template void set_selected_points<TYPENAME>(                \
      SelectedPoints<TYPENAME> & sp, const Field<TYPENAME>& f,            \
      const PointsSelection& psel, const int m);                          \
                                                                          \
  QLAT_EXTERN template void set_selected_points<TYPENAME>(                \
      SelectedPoints<TYPENAME> & sp, const SelectedPoints<TYPENAME>& sp0, \
      const PointsSelection& psel, const PointsSelection& psel0,          \
      const bool is_keeping_data);                                        \
                                                                          \
  QLAT_EXTERN template void set_field_selected<TYPENAME>(                 \
      Field<TYPENAME> & f, const SelectedPoints<TYPENAME>& sp,            \
      const Geometry& geo_, const PointsSelection& psel,                  \
      const bool is_keeping_data);                                        \
                                                                          \
  QLAT_EXTERN template void set_field_selected<TYPENAME>(                 \
      Field<TYPENAME> & f, const SelectedPoints<TYPENAME>& sp,            \
      const Geometry& geo_, const PointsSelection& psel, const int m,     \
      const bool is_keeping_data);                                        \
                                                                          \
  QLAT_EXTERN template void acc_field<TYPENAME>(                          \
      Field<TYPENAME> & f, const SelectedPoints<TYPENAME>& sp,            \
      const Geometry& geo_, const PointsSelection& psel);                 \
                                                                          \
  QLAT_EXTERN template void field_glb_sum_tslice<TYPENAME>(               \
      SelectedPoints<TYPENAME> & sp, const Field<TYPENAME>& f,            \
      const int t_dir);                                                   \
                                                                          \
  QLAT_EXTERN template void lat_data_from_selected_points<TYPENAME>(      \
      LatData & ld, const SelectedPoints<TYPENAME>& sp);                  \
                                                                          \
  QLAT_EXTERN template void selected_points_from_lat_data<TYPENAME>(      \
      SelectedPoints<TYPENAME> & sp, const LatData& ld);                  \
                                                                          \
  QLAT_EXTERN template void save_selected_points<TYPENAME>(               \
      const SelectedPoints<TYPENAME>& sp, QFile& qfile);                  \
                                                                          \
  QLAT_EXTERN template void load_selected_points<TYPENAME>(               \
      SelectedPoints<TYPENAME> & sp, QFile & qfile);                      \
                                                                          \
  QLAT_EXTERN template void save_selected_points<TYPENAME>(               \
      const SelectedPoints<TYPENAME>& sp, const std::string& fn);         \
                                                                          \
  QLAT_EXTERN template void load_selected_points<TYPENAME>(               \
      SelectedPoints<TYPENAME> & sp, const std::string& fn)

QLAT_CALL_WITH_TYPES(QLAT_EXTERN_TEMPLATE);
#undef QLAT_EXTERN_TEMPLATE

#undef QLAT_EXTERN

}  // namespace qlat
