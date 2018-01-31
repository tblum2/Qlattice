#pragma once

#include <qlat/qcd.h>
#include <qlat/qcd-utils.h>

QLAT_START_NAMESPACE

struct GaugeTransform : FieldM<ColorMatrix,1>
{
  virtual const std::string& cname()
  {
    static const std::string s = "GaugeTransform";
    return s;
  }
};

struct U1GaugeTransform: FieldM<ComplexF, 1>
{
	virtual const std::string& cname()
	{
  		static const std::string s = "U1GaugeTransform";
  		return s;
	}
};

inline void gt_apply_gauge_transformation(GaugeTransform& gt0, const GaugeTransform& gt1)
  // gt0 can be the same as gt1
  // gt0 <- gt1 * gt0
{
  TIMER("gt_apply_gauge_transformation");
  assert(is_matching_geo_mult(gt0.geo, gt1.geo));
  const Geometry& geo = gt0.geo;
#pragma omp parallel for
  for (long index = 0; index < geo.local_volume(); ++index) {
    const Coordinate xl = geo.coordinate_from_index(index);
    const ColorMatrix& t1 = gt1.get_elem(xl);
    ColorMatrix& t0 = gt0.get_elem(xl);
    t0 = t1 * t0;
  }
}

inline void gf_apply_gauge_transformation_no_comm(GaugeField& gf, const GaugeField& gf0, const GaugeTransform& gt)
  // gf can be the same as gf0
  // assuming comm for gt is done
  // gf <- gt * gf0
{
  TIMER("gf_apply_gauge_transformation_no_comm");
  assert(is_matching_geo(gf0.geo, gt.geo));
  const Geometry& geo = gf0.geo;
  gf.init(geo_resize(geo, 0));
  assert(is_matching_geo(gf.geo, gf0.geo));
#pragma omp parallel for
  for (long index = 0; index < geo.local_volume(); ++index) {
    Coordinate xl = geo.coordinate_from_index(index);
    Vector<ColorMatrix> v = gf.get_elems(xl);
    const Vector<ColorMatrix> v0 = gf0.get_elems_const(xl);
    const ColorMatrix& t0 = gt.get_elem(xl);
    for (int m = 0; m < DIMN; ++m) {
      xl[m] += 1;
      const ColorMatrix& t1 = gt.get_elem(xl);
      v[m] = t0 * v0[m] * matrix_adjoint(t1);
      xl[m] -= 1;
    }
  }
}

inline void gf_apply_gauge_transformation(GaugeField& gf, const GaugeField& gf0, const GaugeTransform& gt)
{
  TIMER("gf_apply_gauge_transformation");
  assert(is_matching_geo(gf0.geo, gt.geo));
  GaugeTransform gt1;
  gt1.init(geo_resize(gt.geo, 1));
  gt1 = gt;
  refresh_expanded(gt1);
  gf_apply_gauge_transformation_no_comm(gf, gf0, gt1);
}

inline void gf_apply_rand_gauge_transformation(GaugeField& gf, const GaugeField& gf0, const RngState& rs)
{
  const Geometry geo = geo_reform(gf0.geo);
  GaugeTransform gt;
  gt.init(geo);
  set_g_rand_color_matrix_field(gt, rs, 1.0);
  gf_apply_gauge_transformation(gf, gf0, gt);
}

inline void make_temporal_gauge_transformation(GaugeTransform& gt, const GaugeField& gf,
    const int tgref = 0, const int dir = 3)
  // after tranform: ``gf.get_elem(xl, dir) = unit'' is true from ``xg[dir] = tgref''
  // until as far as possible
  // ``gt.get_elem(xl) = unit'' if ``xg[dir] = tgref''
{
  TIMER("make_temporal_gauge_transformation");
  const Geometry geo = geo_reform(gf.geo, 0);
  gt.init(geo);
  assert(is_matching_geo(gt.geo, gf.geo));
  Coordinate expension_left, expension_right;
  set_zero(expension_left);
  set_zero(expension_right);
  expension_left[dir] = 1;
  const Geometry geo1 = geo_resize(geo, expension_left, expension_right);
  GaugeField gf1;
  gf1.init(geo1);
  gf1 = gf;
  refresh_expanded(gf1);
  GaugeTransform gt1;
  gt1.init(geo1);
  set_unit(gt1);
  const Coordinate total_site = geo.total_site();
  for (int tgrel = 1; tgrel < total_site[dir]; ++tgrel) {
    refresh_expanded(gt1);
    const int tg = mod(tgref + tgrel, total_site[dir]);
#pragma omp parallel for
    for (long index = 0; index < geo.local_volume(); ++index) {
      Coordinate xl = geo.coordinate_from_index(index);
      Coordinate xg = geo.coordinate_g_from_l(xl);
      if (tg == xg[dir]) {
        ColorMatrix& t1 = gt1.get_elem(xl);
        xl[dir] -= 1;
        const ColorMatrix& v = gf1.get_elem(xl, dir);
        const ColorMatrix& t0 = gt1.get_elem(xl);
        t1 = t0 * v;
      }
    }
  }
  gt = gt1;
}

inline void make_tree_gauge_transformation(GaugeTransform& gt, const GaugeField& gf,
    const Coordinate& xgref = Coordinate(0, 0, 0, 0), const Coordinate& dirs = Coordinate(0, 1, 2, 3))
{
  TIMER("make_tree_gauge_transformation");
  const Geometry& geo = geo_reform(gf.geo);
  if (false == is_initialized(gt)) {
    gt.init(geo);
  }
  assert(is_matching_geo(gt.geo, gf.geo));
  set_unit(gt);
  GaugeTransform gt_dir;
  gt_dir.init(geo);
  GaugeField gft;
  gft.init(geo);
  gft = gf;
  for (int m = 0; m < DIMN; ++m) {
    make_temporal_gauge_transformation(gt_dir, gft, xgref[dirs[m]], dirs[m]);
    gf_apply_gauge_transformation(gft, gft, gt_dir);
    gt_apply_gauge_transformation(gt, gt_dir);
  }
}

struct FourInterval: std::pair<Coordinate, Coordinate>
{
	virtual const std::string& cname()
	{
  		static const std::string s = "FourInterval";
  		return s;
	}
};

inline void make_local_deflation_plan(std::vector<U1GaugeTransform>& u1gt, 
										std::vector<FourInterval>& global_partition, 
										const Geometry& geo, const Coordinate& tw_par)
	// tw_par indicates the partition number in each of the directions.
	// Should be large or equal than 1. 1 means boundary condition is not changed in this direction.
	// u1gt constains the U1 gauge transformation(s) that move the boundaries to approriate places.
{
	// total number of partition of the global lattice
	int Np = tw_par.product();
	Printf("Number of partitions = %d\n", Np);

	Coordinate global_size = geo.global_size();
	assert( global_size % tw_par == Coordinate(0,0,0,0) );

	assert( geo.eo == 1 ); // Only work with odd sites, for now
	assert( geo.multiplicity == 1 ); // Only work with odd sites, for now

	Coordinate partition_size = global_size / tw_par;

	// initialize the U1 field
	u1gt.resize(Np);
	for(int i = 0; i < Np; i++){
		u1gt[i].init(geo);
		for(size_t j = 0; j < u1gt[i].field.size(); j++){
			u1gt[i].field[j] = 1.;
		}
	}

	// initialize the global_partition.
	global_partition.resize(Np);

	for(int i = 0; i < Np; i++){
		Coordinate partition_coor = qlat::coordinate_from_index(i, tw_par);
		for(int mu = 0; mu < 4; mu++){
			global_partition[i].first[mu] = partition_coor[mu]*partition_size[mu];
			global_partition[i].second[mu] = (partition_coor[mu]+1)*partition_size[mu]; // left(first) including, right(second) excluding. [first, second)
			
			// Find the border that is fartest from the source(partition)
			int target_border = ( global_partition[i].second[mu] + (global_size[mu]-partition_size[mu])/2 ) % global_size[mu];

			Printf("direction = %03d, first = %03d, second = %03d, target = %03d\n", mu, global_partition[i].first[mu], global_partition[i].second[mu], target_border);

			// Flip all gt link in [0, target_border) to -1
#pragma omp parallel for			
			for(size_t index = 0; index < geo.local_volume(); index++){ // We are only working with vectors so it seems we don't need to worry about communication?
				Coordinate local_coor = geo.coordinate_from_index(index);
				Coordinate global_coor = geo.coordinate_g_from_l(local_coor);
				if(global_coor[mu] >= 0 && global_coor[mu] < target_border){
					u1gt[i].field[geo.offset_from_coordinate(local_coor)] *= -1.;
				}
			}
		}
		
	}
}

inline void apply_u1_gauge_tranform_on_bfm_vct(void* bfm_vct, size_t Ls, const U1GaugeTransform& u1gt){
	// Assuming single precision.
	// Since this is only a U1 tranformation we treat spin and color equally.
	// Also need to take into account special indice order of bfm.
	// Assuming bfm_vct is using even-odd preconditioning.
	size_t bfm_vct_block_size = Ls * 12; // 12 = 3 * 4. bfm_vct_block_size is the number single precision complex number on one 4d site.
	ComplexF* bfm_vct_ComplexF = (ComplexF*)bfm_vct;
	size_t site_size_4d = u1gt.geo.local_volume();
#pragma omp parallel for
	for(size_t m = 0; m < site_size_4d/2; m++){
		size_t b1 = m*2;
		size_t b2 = m*2+1;
		size_t m1 = m;
		size_t m2 = m+site_size_4d/2;
		for(size_t s = 0; s < bfm_vct_block_size; s++){
			bfm_vct_ComplexF[b1*bfm_vct_block_size+s] *= u1gt.field[m1];
			bfm_vct_ComplexF[b2*bfm_vct_block_size+s] *= u1gt.field[m2];
		}
	}
}

inline bool is_inside(const Coordinate& coor, const Coordinate& lower, const Coordinate& upper){
	return (lower[0] <= coor[0] and coor[0] < upper[0]) and
			(lower[1] <= coor[1] and coor[1] < upper[1]) and
			(lower[2] <= coor[2] and coor[2] < upper[2]) and
			(lower[3] <= coor[3] and coor[3] < upper[3]);
}

inline void extract_par_vct_from_bfm_vct(void* par_vct, const void* bfm_vct, size_t Ls, 
											const FourInterval& par, const Geometry& geo){
	// Assuming single precision.
	// Also need to take into account special indice order of bfm.
	// Assuming bfm_vct is using even-odd preconditioning.
	size_t bfm_vct_block_size = Ls * 12; // 12 = 3 * 4. bfm_vct_block_size is the number of single precision complex number on one 4d site.
	size_t bfm_vct_block_byte_size = Ls * 12 * sizeof(ComplexF);
	size_t site_size_4d = geo.local_volume();
#pragma omp parallel for
	for(size_t m = 0; m < site_size_4d/2; m++){
		size_t b1 = m*2;
		size_t b2 = m*2+1;
		size_t m1 = m;
		size_t m2 = m+site_size_4d/2;

		Coordinate local_coor1 = geo.coordinate_from_index(m1);
		Coordinate global_coor1 = geo.coordinate_g_from_l(local_coor1);
		Coordinate local_coor2 = geo.coordinate_from_index(m2);
		Coordinate global_coor2 = geo.coordinate_g_from_l(local_coor2);

		if( is_inside(global_coor1, par.first, par.second) ){
			memcpy(par_vct+b1*bfm_vct_block_byte_size, bfm_vct+b1*bfm_vct_block_byte_size, bfm_vct_block_byte_size);
		}else{
			memset(par_vct+b1*bfm_vct_block_byte_size, 0, bfm_vct_block_byte_size);
		}

		if( is_inside(global_coor2, par.first, par.second) ){
			memcpy(par_vct+b2*bfm_vct_block_byte_size, bfm_vct+b2*bfm_vct_block_byte_size, bfm_vct_block_byte_size);
		}else{
			memset(par_vct+b2*bfm_vct_block_byte_size, 0, bfm_vct_block_byte_size);
		}
	}
}

QLAT_END_NAMESPACE
