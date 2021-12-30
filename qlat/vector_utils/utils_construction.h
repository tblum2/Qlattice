// utils_construction.h
// Gen Wang
// Jul. 2021

#ifndef UTILS_CONSTRUCTION_H
#define UTILS_CONSTRUCTION_H

#pragma once

#include "float_type.h"
#include "gammas.h"
#include "utils_momentum.h"
#include "fft_desc.h"
#include "utils_reduce_vec.h"
#include "utils_grid_src.h"

namespace qlat{

#ifdef QLAT_USE_ACC
#define USEKERNEL 1
#define USEGLOBAL 1
#define USEQACC   1
#else
#define USEKERNEL 0
#define USEGLOBAL 0
#define USEQACC   0
#endif

template<typename Ty>
void clear_qv(qlat::vector_acc<Ty > &G)
{
  zero_Ty(G.data(), G.size(), 1 );
  //////qacc_for(i, long(G.size()),{G[i] = 0; });
  /////for(int i = 0;i< G.size(); i++){G[i] = 0.0;}
}

//template<typename Ty1, typename Ty2>
//inline void cp_C(Ty1 a, Ty2 b)
//{
//  if(typeid(Ty1) == typeid(Ty2)){a = b;}
//  else{
//    a = Ty1(b.real, b.imag);
//  }
//}

void prop4d_src_gamma(Propagator4d& prop, ga_M& ga,int dir = 0)
{
  ////Rowmajor (a,b), b is continues in memory
  qacc_for(isp, long(prop.geo().local_volume()),{ 
    qlat::WilsonMatrix& v0 =  prop.get_elem(isp);
    qlat::WilsonMatrix  v1 = v0;

    for (int s = 0; s < 4; ++s)
    for(int c0 = 0;c0< 3 ; c0++)
    for (int d0 = 0; d0 < 4; ++d0)
    {
      /////Source multiply 
      if(dir==0)for(int c1=0;c1<3;c1++)v0(s*3 + c0, ga.ind[d0]*3 + c1) = ga.g[d0] * v1(s*3 + c0, d0*3 + c1);
      /////Sink multiply 
      if(dir==1)for(int c1=0;c1<3;c1++)v0(d0*3 + c0, s*3 + c1) = ga.g[d0] * v1(ga.ind[d0]*3 + c0, s*3 + c1);

      ///////Source multiply 
      //if(dir==0)for(int c1=0;c1<3;c1++)cp_C(v0(s*3 + c0, ga.ind[d0]*3 + c1), ga.g[d0] * v1(s*3 + c0, d0*3 + c1));
      ///////Sink multiply 
      //if(dir==1)for(int c1=0;c1<3;c1++)cp_C(v0(d0*3 + c0, s*3 + c1), ga.g[d0] * v1(ga.ind[d0]*3 + c0, s*3 + c1));
    }
  });
}

void prop4d_sink_gamma(Propagator4d& prop, ga_M& ga)
{
  prop4d_src_gamma(prop, ga ,1);
}

void prop4d_cps_to_ps(Propagator4d& prop, int dir=0)
{
  /////sn is -1 for default
  double sn =-1;if(dir == 1){sn= 1;}
  const double sqrt2=std::sqrt(2.0);

  ////Rowmajor (a,b), b is continues in memory
  qacc_for(isp, prop.geo().local_volume(),{ 
    qlat::WilsonMatrix  v0 = prop.get_elem(isp);
    qlat::WilsonMatrix  v1 = prop.get_elem(isp);

    int dr,d0,d1;
    /////Src rotation
    for (int s0 = 0; s0 < 4; ++s0)
    for(int c0 = 0;c0< 3 ; c0++)
    {
      dr=0;d0=1;d1=3;
      for(int c1=0;c1<3;c1++)v0(s0*3+c0, dr*3+c1) = (-v1(s0*3+c0, d0*3+ c1)*sn + v1(s0*3+c0, d1*3+c1)   )/sqrt2;
      dr=1;d0=0;d1=2;
      for(int c1=0;c1<3;c1++)v0(s0*3+c0, dr*3+c1) = (+v1(s0*3+c0, d0*3+ c1)*sn - v1(s0*3+c0, d1*3+c1)   )/sqrt2;
      dr=2;d0=1;d1=3;
      for(int c1=0;c1<3;c1++)v0(s0*3+c0, dr*3+c1) = (-v1(s0*3+c0, d0*3+ c1)    - v1(s0*3+c0, d1*3+c1)*sn)/sqrt2;
      dr=3;d0=0;d1=2;
      for(int c1=0;c1<3;c1++)v0(s0*3+c0, dr*3+c1) = (+v1(s0*3+c0, d0*3+ c1)    + v1(s0*3+c0, d1*3+c1)*sn)/sqrt2;
    }

    /////Copy previous results
    v1 = v0;
    /////Sink rotation
    for(int c0 = 0;c0< 3 ; c0++)
    for (int s0 = 0; s0 < 4; ++s0)
    {
      dr=0;d0=1;d1=3;
      for(int c1=0;c1<3;c1++)v0(dr*3+c0, s0*3+c1) = (-v1(d0*3+c0, s0*3+c1)*sn + v1(d1*3+c0, s0*3+c1)   )/sqrt2;
      dr=1;d0=0;d1=2;
      for(int c1=0;c1<3;c1++)v0(dr*3+c0, s0*3+c1) = ( v1(d0*3+c0, s0*3+c1)*sn - v1(d1*3+c0, s0*3+c1)   )/sqrt2;
      dr=2;d0=1;d1=3;
      for(int c1=0;c1<3;c1++)v0(dr*3+c0, s0*3+c1) = (-v1(d0*3+c0, s0*3+c1)    - v1(d1*3+c0, s0*3+c1)*sn)/sqrt2;
      dr=3;d0=0;d1=2;
      for(int c1=0;c1<3;c1++)v0(dr*3+c0, s0*3+c1) = ( v1(d0*3+c0, s0*3+c1)    + v1(d1*3+c0, s0*3+c1)*sn)/sqrt2;
    }
    prop.get_elem(isp) = v0;

  });
}

void prop4d_ps_to_cps(Propagator4d& prop)
{
  prop4d_cps_to_ps(prop, 1);
}

void get_corr_pion(std::vector<qlat::FermionField4dT<qlat::Complex> > &prop,const Coordinate &x_ini, std::vector<double > &write ){

  const qlat::Geometry &geo = prop[0].geo();

  unsigned long Nvol = geo.local_volume();
  ///int Nt = geo.node_site[3];
  ///long Nsum = Nvol/Nt;
  int tini = x_ini[3];

  qlat::vector_acc<qlat::Complex > res;res.resize(Nvol);

  qacc_for(isp, long(Nvol),{
    qlat::Complex buf(0.0,0.0);

    for(int dc2=0;dc2<12;dc2++){
      qlat::Complex* a = (qlat::Complex* ) &(prop[dc2].get_elem(isp));
      for(int dc1=0;dc1<12;dc1++)
      {
        buf+=a[dc1]*qlat::qconj(a[dc1]);
      }
    }
    res[isp] = buf;
    ////src[isp] = buf[isp];
  });

  const Coordinate vg = geo.total_site();
  int nt = vg[3];
  write.resize(0);
  write.resize(2*nt);qlat::set_zero(write);

  for(unsigned long isp=0;isp<Nvol;isp++){
    Coordinate xl0 = geo.coordinate_from_index(isp);
    Coordinate xg0 = geo.coordinate_g_from_l(xl0);
    int t = xg0[3];

    int toff = ((t-tini+nt)%nt);
    write[ toff*2 + 0 ] += res[isp].real();
    write[ toff*2 + 1 ] += res[isp].imag();
  }
  ////May need to be changed for EigenV
  //sum_all_size((double*) &write[0],2*nt);
  sum_all_size((double*) write.data(), 2*nt);
}

void vec_corrE(EigenV &resE, EigenV &res,qlat::fft_desc_basic &fd,int clear=0,int imom=505050)
{
  int NTt  = fd.Nv[3];
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int nmass = resE.size()/(NTt*Nxyz);


  ////position p = fd.desc.get_position(0,fd.rank);
  int t_rank = fd.Pos0[fd.rank][3];

  if(imom != 505050)
  {
    int mom[3];
    qlat::momentum tem_mom(imom);
    tem_mom.ind_2_mom(mom);
    double p0[3]={2*3.1415926535898/fd.nx,
                  2*3.1415926535898/fd.ny,
                  2*3.1415926535898/fd.nz};

    Evector phaseE;phaseE.resize(0);phaseE.resize(Nxyz);

    qlat::vector_gpu<int > pos_tem;pos_tem.copy_from(fd.Pos0[fd.rank]);int* posP = pos_tem.data();

    qacc_for(xi, long(Nxyz),{
      int pi[3];
      pi[fd.orderN[0]] = xi/(fd.Nv[fd.orderN[1]]*fd.Nv[fd.orderN[2]]);
      pi[fd.orderN[1]] = (xi%(fd.Nv[fd.orderN[1]]*fd.Nv[fd.orderN[2]]))/fd.Nv[fd.orderN[2]];
      pi[fd.orderN[2]] = xi%fd.Nv[fd.orderN[2]];
      for(int ptem=0;ptem<3;ptem++){pi[ptem] = pi[ptem] + posP[ptem];}

      double theta=mom[0]*p0[0]*pi[0]+mom[1]*p0[1]*pi[1]+mom[2]*p0[2]*pi[2];
      phaseE[xi] = Complexq(cos(theta),sin(theta));
    });

    size_t Ns = Nxyz;
    Ns = nmass*NTt*Nxyz;
    qacc_for(i, long(Ns),{
      LInt mi = i/(NTt*Nxyz);
      LInt ti = (i%(NTt*Nxyz))/Nxyz;
      LInt xi = i%(Nxyz);

      //resEp[mi*NTt + ti][xi] = resEp[mi*NTt + ti][xi]*phaseE[xi];
      resE[(mi*NTt + ti)*Nxyz + xi] = resE[(mi*NTt + ti)*Nxyz + xi]*phaseE[xi];
    });
    /////#endif
  }

  ////TODO
  int nt = fd.nt;
  //if(clear == 1){res.resize(0);res.resize(nmass*nt);qlat::set_zero(res);}
  if(clear == 1){if(res.size() != nmass*nt){res.resize(nmass*nt);} qlat::set_zero(res);}
  if(clear == 0){if(res.size() != nmass*nt){print0("res size wrong for corr.");qassert(false);}}

  qlat::vector_acc<Complexq > tmp;tmp.resize(nmass*NTt);qlat::set_zero(tmp);//tmp.set_zero();
  reduce_vec(resE.data(), tmp.data(), Nxyz, nmass*NTt);

  qlat::vector_gpu<Complexq > RES;RES.resize(nmass*nt );RES.set_zero();
  Complexq* s1 = RES.data();Complexq* s0 = tmp.data();
  long Ntotal = nmass*NTt;
  qacc_for(mti, Ntotal, {
    long mi  = mti/NTt;
    long  ti = mti%NTt;
    s1[mi*nt + t_rank + ti ] = s0[mi*NTt + ti];
  });

  ////sum_all_size((Ftype*) (RES.data()), 2*RES.size(), 1);
  sum_all_size(RES.data(), RES.size(), 1);

  cpy_data_thread(res.data(), RES.data(), RES.size(), 1, true, 1.0);

}

void shift_result_t(EigenV& Esrc, int nt, int tini)
{
  if(tini == 0){return ;}
  long Ntotal = Esrc.size();
  if(Ntotal %(nt) != 0){abort_r("Correlation function size wrong!\n");}
  EigenV tmp;tmp.resize(Ntotal);
  qacc_for(i, Ntotal, {
    const int iv = i/nt;
    const int t  = i%nt;
    tmp[iv*nt + (t - tini + nt)%nt] = Esrc[iv*nt + (t)%nt];
  });
  cpy_data_thread(Esrc.data(), tmp.data(), tmp.size(), 1);
}

//void clear_EigenM(EigenM &res)
//{
//  for(int iv=0;iv<res.size();iv++)
//  {
//    zero_Ty(res[iv].data(), res[iv].size(), 1, false);
//  }
//  qacc_barrier(dummy);
//}

void ini_propE(EigenM &prop,int nmass, qlat::fft_desc_basic &fd, bool clear = true)
{
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int NTt  = fd.Nv[3];
  int do_resize = 0;
  if(prop.size() != (LInt) (nmass*12*12*NTt)){do_resize = 1;}
  for(unsigned int i=0;i<prop.size();i++){if((LInt) prop[i].size() != Nxyz){do_resize=1;}}

  if(do_resize == 1)
  {
    for(unsigned int i=0;i<prop.size();i++){prop[i].resize(0);}
    prop.resize(0);
    prop.resize(nmass*12*12*NTt);
    for(unsigned int i=0;i<prop.size();i++){
      prop[i].resize(Nxyz);
    }
  }

  if(clear){zeroE(prop);}
}

template<typename Ty>
void copy_propE(Propagator4dT<Ty > &pV1,EigenM &prop, qlat::fft_desc_basic &fd, int dir=0)
{
  TIMER("Copy prop");
  qassert(fd.order_ch == 0);
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int NTt  = fd.Nv[3];
  int nmass = 1;
  if(dir==0){ini_propE(prop,nmass,fd);}
  if(dir==1){
    nmass = prop.size()/(12*12*NTt);
    if(!pV1.initialized){
      Geometry geo;fd.get_geo(geo);
      pV1.init(geo);
    }
  }

  for(int mi = 0;mi < nmass;mi++)
  {
    Propagator4dT<Ty >& pv = pV1;
    qlat::vector_acc<Complexq* > propP = EigenM_to_pointers(prop);
    qacc_for(isp, long(NTt*Nxyz),{ 
      int ti = isp/Nxyz;
      int xi = isp%Nxyz;

        qlat::WilsonMatrixT<Ty>& v0 =  pv.get_elem(isp);

        for(int c0 = 0;c0 < 3; c0++)
        for(int d0 = 0;d0 < 4; d0++)
        for(int c1 = 0;c1 < 3; c1++)
        for(int d1 = 0;d1 < 4; d1++)
        {
          LInt off = mi*12*12 + (d1*3+c1)*12+d0*3+c0;
          if(dir==0)propP[off*NTt+ti][xi] = v0(d0*3 + c0, d1*3 + c1);
          if(dir==1)v0(d0*3 + c0, d1*3 + c1) = propP[off*NTt+ti][xi];
        }
    });
  }

}


template<typename Ty>
void copy_propE(std::vector<Propagator4dT<Ty > > &pV1,EigenM &prop, qlat::fft_desc_basic &fd, int dir=0)
{
  TIMER("Copy prop");
  qassert(fd.order_ch == 0);
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int NTt  = fd.Nv[3];
  int nmass = 0;
  if(dir==0){nmass = pV1.size();ini_propE(prop,nmass,fd);}
  if(dir==1){
    nmass = prop.size()/(12*12*NTt);
    if(pV1.size() != (LInt) nmass){
      pV1.resize(0);
      Geometry geo;fd.get_geo(geo);
      pV1.resize(nmass);for(int i=0;i<nmass;i++){pV1[i].init(geo);}
    }
  }

  for(int mi = 0;mi < nmass;mi++)
  {
    Propagator4dT<Ty >& pv = pV1[mi];
    qlat::vector_acc<Complexq* > ps = EigenM_to_pointers(prop);
    qacc_for(isp, long(NTt*Nxyz),{ 
      int ti = isp/Nxyz;
      int xi = isp%Nxyz;

        //qlat::WilsonMatrix& v0 =  pv.get_elem(isp);
        qlat::WilsonMatrixT<Ty>& v0 =  pv.get_elem(isp);

        for(int c0 = 0;c0 < 3; c0++)
        for(int d0 = 0;d0 < 4; d0++)
        for(int c1 = 0;c1 < 3; c1++)
        for(int d1 = 0;d1 < 4; d1++)
        {
          LInt off = mi*12*12 + (d1*3+c1)*12+d0*3+c0;
          if(dir==0)ps[off*NTt+ti][xi] = v0(d0*3 + c0, d1*3 + c1);
          if(dir==1)v0(d0*3 + c0, d1*3 + c1) = ps[off*NTt+ti][xi];
        }
    });
  }

}


void check_prop_size(EigenM &prop)
{
  int sizep = prop.size();
  if(sizep%(12*12) != 0 or sizep == 0)
  {
    print0("Size of Prop wrong. \n");
    qassert(false);
    ///shutdown_machine();
    ///abort();
  }
}

void ini_resE(EigenV &res,int nmass, qlat::fft_desc_basic &fd)
{
  int NTt  = fd.Nv[3];
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int do_resize = 0;
  if((LInt) res.size() != (LInt) nmass*NTt * Nxyz){do_resize=1;}
  if(do_resize == 1)
  {
    res.resize(0);res.resize(nmass*NTt * Nxyz);
  }

  clear_qv(res);
}

void meson_vectorE(std::vector<Propagator4d > &pV1, std::vector<Propagator4d > &pV2, ga_M &ga1,ga_M &ga2,
        EigenV &res, qlat::fft_desc_basic &fd,int clear=1)
{
  TIMER("Meson_vectorE");
  qassert(fd.order_ch == 0);
  ///////check_prop_size(prop1);check_prop_size(prop2);
  int  NTt  = fd.Nv[3];
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int  nmass = pV1.size();
  if(nmass == 0){res.resize(0);return;}

  if(clear == 1){ini_resE(res,nmass,fd);}

  if(res.size()%NTt !=0 or res.size()==0){print0("Size of res wrong. \n");qassert(false);}
  qassert(pV1.size() == pV2.size());

  for(int mi=0;mi<nmass;mi++)
  {
  Propagator4d& pL1 = pV1[mi];
  Propagator4d& pL2 = pV2[mi];

  qacc_for(isp, long(pV1[0].geo().local_volume()),{ 
    int ti = isp/Nxyz;
    int xi = isp%Nxyz;
      Complexq pres;pres = 0.0;
      const qlat::WilsonMatrix& p1 =  pL1.get_elem(isp);
      const qlat::WilsonMatrix& p2 =  pL2.get_elem(isp);

      for(int d1=0;d1<4;d1++)
      for(int c1=0;c1<3;c1++)
      for(int d2=0;d2<4;d2++)
      {
      Complexq g_tem = ga2.g[d2]*ga1.g[d1];
      for(int c2=0;c2<3;c2++)
      {
        pres += g_tem * 
          p1(ga1.ind[d1]*3+c1,d2*3+c2) * qlat::qconj(p2(d1*3+c1,ga2.ind[d2]*3+c2)) ;
      }
      }
      res[(mi*NTt + ti)*Nxyz + xi%Nxyz] += pres;
  });
  }

}

void meson_vectorE(EigenM &prop1, EigenM &prop2, ga_M &ga1,ga_M &ga2,
        EigenV &res, qlat::fft_desc_basic &fd,int clear=1, int invmode=1)
{
  TIMER("Meson_vectorE");
  check_prop_size(prop1);check_prop_size(prop2);
  ///////check_prop_size(prop1);check_prop_size(prop2);
  int  NTt  = fd.Nv[3];
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int  nmass = prop1.size()/(12*12*NTt);
  if(nmass == 0){res.resize(0);return;}

  if(clear == 1){ini_resE(res,nmass,fd);}

  if(res.size()%NTt !=0 or res.size()==0){print0("Size of res wrong. \n");qassert(false);}
  qassert(prop1.size() == prop2.size());

  for(int d2=0;d2<4;d2++)
  for(int c2=0;c2<3;c2++)
  for(int d1=0;d1<4;d1++)
  for(int c1=0;c1<3;c1++)
  {
  #pragma omp parallel for
  for(int ji=0;ji<nmass*NTt;ji++)
  {
    int massi = ji/NTt;
    int ti    = ji%NTt;

    int off1 = massi*12*12 + (d2*3+c2)*12+ga1.ind[d1]*3+c1;
    int off2 = massi*12*12 + (ga2.ind[d2]*3+c2)*12+d1*3+c1;

    Complexq g_tem = ga2.g[d2]*ga1.g[d1];

    Complexq* tp1 = prop1[off1*NTt+ti].data();
    Complexq* tp2 = prop2[off2*NTt+ti].data();
    Complexq* tr0 = &((res.data())[(massi*NTt + ti)*Nxyz]);

    #if USEQACC==1
    if(invmode == 1){qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*qlat::qconj(tp2[i]) * g_tem);});}
    if(invmode == 0){qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*           (tp2[i]) * g_tem);});}
    #else
    EA vp1(tp1,Nxyz);
    EA vp2(tp2,Nxyz);
    EA vr0(tr0,Nxyz);
    if(invmode == 1)vr0 += (vp1*vp2.conjugate() * g_tem);
    if(invmode == 0)vr0 += (vp1*vp2             * g_tem);
    #endif

    ////EA vp1(&prop1[off1*NTt+ti][0],Nxyz);
    ////EA vp2(&prop2[off2*NTt+ti][0],Nxyz);
    ////EA vr0(&res[mi*NTt + ti][0],Nxyz);
  }
  qacc_barrier(dummy);
  }

}


void meson_corrE(std::vector<Propagator4d > &pV1, std::vector<Propagator4d > &pV2,  ga_M &ga1, ga_M &ga2,
  EigenV &res, qlat::fft_desc_basic &fd,int clear=1,int imom=505050,int mode_GPU = 0)
{
  ///int NTt  = fd.Nv[3];
  ///LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int nmass = pV1.size();
  ///int nt = fd.nt;

  EigenV resE;
  ini_resE(resE,nmass,fd);

  if(mode_GPU == 0){
    EigenM prop1;
    EigenM prop2;
    copy_propE(pV1,prop1, fd);
    copy_propE(pV2,prop2, fd);
    meson_vectorE(prop1,prop2,ga1,ga2,resE,fd,1);
  }
  if(mode_GPU == 1){meson_vectorE(pV1,pV2,ga1,ga2,resE,fd,1);}

  vec_corrE(resE,res,fd, clear, imom);
}

void meson_corrE(EigenM &prop1,EigenM &prop2,  ga_M &ga1, ga_M &ga2,
  EigenV &res, qlat::fft_desc_basic &fd,int clear=1,int imom=505050)
{
  ///int NTt  = fd.Nv[3];
  ////LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  ///int  nmass = prop1.size()/(12*12*NTt);
  ////int nt = fd.nt;

  EigenV resE;
  ////ini_resE(resE,nmass,fd);

  meson_vectorE(prop1,prop2,ga1,ga2,resE,fd,1);
  vec_corrE(resE,res,fd, clear, imom);
}


///////Proton contractions

void proton_vectorE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
  ga_M &ga2,int ind2, ga_M &ga1,int ind1,
        EigenV &res, fft_desc_basic &fd,int clear=1)
{
  TIMER("Proton_vectorE");
  ////ga2/ind2 for source, gam1/ind1 for sink
  ////"[]|+N" type diagram
  check_prop_size(prop1);check_prop_size(prop2);check_prop_size(prop3);
  int NTt  = fd.Nv[3];
  LInt Nxyz = prop1[0].size();
  int nmass = prop1.size()/(12*12*NTt);
  qassert(prop1.size() == prop2.size());
  qassert(prop1.size() == prop3.size());
  if(clear == 1){ini_resE(res,nmass,fd);}
    
  ////Prop format, src d-4, c-3, sink d-4, c-3, Nt, EigenV<Nxyz>
  if(res.size()%NTt !=0 or res.size()==0){print0("Size of res wrong. \n");qassert(false);}

  Ftype epsl[3][3];
  for(int i=0;i<3;i++){epsl[i][i]=0;epsl[i][(i+1)%3]=1;epsl[i][(i+2)%3]=-1;}

  for(int d2=0;d2<4;d2++)
  for(int c21=0;c21<3;c21++)
  for(int ib=1;ib<3;ib++)
  {
    int c22=(c21+ib)%3,c23=(c22+ib)%3;
    for(int d1=0;d1<4;d1++)
    for(int c11=0;c11<3;c11++)
    for(int ia=1;ia<3;ia++)
    {
      int c12=(c11+ia)%3,c13=(c12+ia)%3;
      //double_complex gi_tem = epsl[c11][c12]*epsl[c21][c22]*ga1.g[d1]*ga2.g[d2];
      //std::complex<Ftype> giE(gi_tem.real,gi_tem.imag);
      Complexq giE = epsl[c11][c12]*epsl[c21][c22]*ga1.g[d1]*ga2.g[d2];

      #pragma omp parallel for
      for(int ji=0;ji<nmass*NTt;ji++)
      {
        int massi = ji/NTt;
        int ti    = ji%NTt;

        int m1 = massi*12*12 + (ind2*3+c21)*12+ind1*3+c11;
        int m2 = massi*12*12 + (ga2.ind[d2]*3+c22)*12+d1*3+c12;
        int m3 = massi*12*12 + (d2*3+c23)*12+ga1.ind[d1]*3+c13;

        int n1 = massi*12*12 + (ind2*3+c21)*12+ga1.ind[d1]*3+c11;
        int n2 = massi*12*12 + (ga2.ind[d2]*3+c22)*12+d1*3+c12;
        int n3 = massi*12*12 + (d2*3+c23)*12+ind1*3+c13;

        Complexq* tp1 = prop1[m1*NTt+ti].data();
        Complexq* tp2 = prop2[m2*NTt+ti].data();
        Complexq* tp3 = prop3[m3*NTt+ti].data();
        Complexq* tn1 = prop1[n1*NTt+ti].data();
        Complexq* tn2 = prop2[n2*NTt+ti].data();
        Complexq* tn3 = prop3[n3*NTt+ti].data();
        Complexq* tr0 = &(res.data()[(massi*NTt + ti)*Nxyz]);

        #if USEQACC==1
        qacc_forNB(i, long(Nxyz),{ tr0[i] -= ((tp1[i]*tp2[i]*tp3[i] + tn1[i]*tn2[i]*tn3[i])*giE); });
        #else
        EA vp1(tp1,Nxyz);
        EA vp2(tp2,Nxyz);
        EA vp3(tp3,Nxyz);
        EA vn1(tn1,Nxyz);
        EA vn2(tn2,Nxyz);
        EA vn3(tn3,Nxyz);
        EA vr0(tr0,Nxyz);
        vr0 -= ((vp1*vp2*vp3 + vn1*vn2*vn3)*giE);
        #endif

        /////EA vp1(&prop1[m1*NTt+ti][0],Nxyz);
        /////EA vp2(&prop2[m2*NTt+ti][0],Nxyz);
        /////EA vp3(&prop3[m3*NTt+ti][0],Nxyz);
        /////EA vn1(&prop1[n1*NTt+ti][0],Nxyz);
        /////EA vn2(&prop2[n2*NTt+ti][0],Nxyz);
        /////EA vn3(&prop3[n3*NTt+ti][0],Nxyz);
        /////EA vr0(&res[massi*NTt + ti][0],Nxyz);

      }
      qacc_barrier(dummy);
    }
  }

}

#ifdef QLAT_USE_ACC
template <typename Ty, int invmode, int bfac, int Blocks>
__global__ void meson_vectorEV_global(Ty** p1, Ty** p2, Ty* resP, 
  char** gPP, unsigned char** oPP, const int* ivP,
  const int nmass, const int NTt, const long Nxyz, const int Ngv)
{
  const unsigned long gi =  blockIdx.x;
  const unsigned int tid = threadIdx.y*blockDim.x+ threadIdx.x;
  const long Ntotal = nmass * NTt * Nxyz;
  const long Nbfac  = Ntotal/bfac;
  __shared__ Ty P1[bfac*12*12];
  __shared__ Ty P2[bfac*12*12];

  if(gi*bfac < Ntotal){

  int bi0= 0;int dc = 0;
  int ji = 0;int massi = 0;int ti = 0;

  int jobN = bfac*12*12;
  unsigned int off = tid;
  while(off < jobN){
    bi0= off/(12*12);
    dc = off%(12*12);
    ji    = (bi0*Nbfac + gi)/Nxyz;
    massi = ji/NTt;
    ti    = ji%NTt;
    long ixyz = (bi0*Nbfac + gi)%Nxyz;
    P1[dc*bfac + bi0] = p1[(massi*12*12 + dc)*NTt + ti][ixyz];
    P2[dc*bfac + bi0] = p2[(massi*12*12 + dc)*NTt + ti][ixyz];
    off += Blocks;
  }
  __syncthreads();

  int ini = 0;
  int dv = 0;
  unsigned int MAX = 0;

  const int bfacC = bfac;
  const int Nth   = Blocks/bfac;
  const unsigned int Each  =  4*bfacC;
  const unsigned int GROUP = (Blocks/bfac)*Each;
  unsigned char* s0 = NULL;
           char* s1 = NULL;

  const int bi =  threadIdx.y;
  const int ai =  threadIdx.x;

  const int ba = (threadIdx.y/bfacC)*bfacC + 0;
  const int aa = (threadIdx.y%bfacC)*blockDim.x + threadIdx.x;

  __shared__ Ty buf[bfacC*Blocks];
  __shared__ unsigned char pos[3*GROUP];
  __shared__ char g0[2*GROUP];

  for(int iv=0;iv<Ngv;iv++)
  {
    for(int bz=0;bz<bfacC;bz++){buf[bz*Blocks + tid] = 0;}
    MAX = ivP[iv];
    jobN = (MAX + GROUP - 1 )/GROUP;
    ini = 0; dv = GROUP;
    for(int ji=0;ji<jobN;ji++){
      ////if(ini >= MAX){continue;}
      if(ini + dv >= MAX){dv = MAX - ini;}
      s0 = &(oPP[iv][ini*2]);
      s1 = &(gPP[iv][ini*2]);

      off = tid;
      while(off < dv*2){pos[off] = s0[off];off += Blocks;}
      off = tid;
      while(off < dv*2){g0[off]  = s1[off];off += Blocks;}
      __syncthreads();

      off = aa;
      while(off < dv){
        const Ty* t1 = &P1[(pos[off*2+0])*bfac + ba];
        const Ty* t2 = &P2[(pos[off*2+1])*bfac + ba];
        Ty gtem = Ty(g0[off*2+0], g0[off*2+1]);
        if(bfacC == 1){
          if(invmode == 0){ buf[aa*bfac+ba] += (t1[0]*           (t2[0]) * gtem); }
          if(invmode == 1){ buf[aa*bfac+ba] += (t1[0]*qlat::qconj(t2[0]) * gtem); }
        }else{
          Ty* b0 = &buf[aa*bfac+ba];
          for(int bz=0;bz<bfacC;bz++){
            if(invmode == 0){b0[bz] += (t1[bz]*           (t2[bz]) * gtem); }
            if(invmode == 1){b0[bz] += (t1[bz]*qlat::qconj(t2[bz]) * gtem); }
          }
        }
        off += Nth*bfacC;
      }
      __syncthreads();

      ini += dv;
    }

    for(int atem=1;atem<bfacC;atem++){buf[(0*Nth+ai)*bfac+bi] += buf[(atem*Nth+ai)*bfac+bi];} __syncthreads();

    if(Nth >=256){if(ai <128){buf[ai*bfac + bi] += buf[(ai+128)*bfac + bi];}__syncthreads();}
    if(Nth >=128){if(ai < 64){buf[ai*bfac + bi] += buf[(ai+ 64)*bfac + bi];}__syncthreads();}
    if(Nth >= 64){if(ai < 32){buf[ai*bfac + bi] += buf[(ai+ 32)*bfac + bi];}__syncthreads();}
    if(Nth >= 32){if(ai < 16){buf[ai*bfac + bi] += buf[(ai+ 16)*bfac + bi];}__syncthreads();}
    if(Nth >= 16){if(ai <  8){buf[ai*bfac + bi] += buf[(ai+  8)*bfac + bi];}__syncthreads();}
    if(Nth >=  8){if(ai <  4){buf[ai*bfac + bi] += buf[(ai+  4)*bfac + bi];}__syncthreads();}
    if(Nth >=  4){if(ai <  2){buf[ai*bfac + bi] += buf[(ai+  2)*bfac + bi];}__syncthreads();}

    if(ai == 0){
      resP[iv*Ntotal + bi*Nbfac + gi] += (buf[bi] + buf[bfac+bi]);
      //if(clear == 0){resP[iv*Ntotal + bi*Nbfac + gi] += (buf[bi] + buf[bfac+bi]);}
      //if(clear == 1){resP[iv*Ntotal + bi*Nbfac + gi]  = (buf[bi] + buf[bfac+bi]);}
    }
    __syncthreads();

  }

  }

}
#endif

template <typename Ty, int invmode, int bfac>
void meson_vectorEV_kernel(Ty** p1, Ty** p2, Ty* resP, 
  char** gPP, unsigned char** oPP, const int* ivP,
  const int nmass, const int NTt, const long Nxyz, const int Ngv)
{
  long Ntotal  = nmass*NTt*Nxyz;
  if(Ntotal % bfac != 0){abort_r("Please correct your bfac! \n");}
  long Nbfac = Ntotal/bfac;
  #if USEGLOBAL==1
  const int nt =  8;
  const int Blocks = nt*bfac;
  dim3 dimBlock(    nt, bfac, 1);
  dim3 dimGrid(  Nbfac,  1, 1);
  meson_vectorEV_global<Complexq, invmode, bfac, Blocks><<<dimGrid, dimBlock>>>(p1, 
        p2, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  qacc_barrier(dummy);
  #else
  if((nmass*NTt) % bfac != 0){abort_r("Please correct your bfac! \n");}
  qacc_for(gi, Nbfac ,
  {
    Ty buf[bfac+1];
    Ty P1[bfac*12*12+1];
    Ty P2[bfac*12*12+1];

    long ixyz = gi%Nxyz;
    int ji    = (gi/Nxyz)*bfac + 0;
    int massi = ji/NTt;
    int ti    = ji%NTt;
    const long offR0 = (massi*NTt + ti)*Nxyz + ixyz;

    for(int bi=0;bi<bfac;bi++)
    {
      massi = (ji+bi)/NTt;
      ti    = (ji+bi)%NTt;

      for(int dc=0;dc<12*12;dc++){
        P1[dc*bfac + bi] = p1[(massi*12*12 + dc)*NTt + ti][ixyz];
        P2[dc*bfac + bi] = p2[(massi*12*12 + dc)*NTt + ti][ixyz];
      }
    }

    for(int iv=0;iv<Ngv;iv++){
      for(int bi=0;bi<bfac;bi++){buf[bi] = 0;}
      for(int off=0;off<ivP[iv];off++)
      {
        const Ty* t1 = &P1[(oPP[iv][off*2+0])*bfac];
        const Ty* t2 = &P2[(oPP[iv][off*2+1])*bfac];
        const Ty gtem = Ty(gPP[iv][off*2+0], gPP[iv][off*2+1]);
        for(int bi=0;bi<bfac;bi++)
        {
          if(invmode == 1){ buf[bi] += (t1[bi]*qlat::qconj(t2[bi]) * gtem); }
          if(invmode == 0){ buf[bi] += (t1[bi]*           (t2[bi]) * gtem); }
        }
      }

      long offR = iv * Ntotal;
      Ty* r0 = &resP[offR + offR0];
      for(int bi=0;bi<bfac; bi++){
        r0[bi*Nxyz] += buf[bi];
        //if(clear == 0){r0[bi*Nxyz] += buf[bi];}
        //if(clear == 1){r0[bi*Nxyz]  = buf[bi];}
      }
    }
  });
  #endif

}

template <typename Ty>
void meson_vectorEV(Ty** p1, Ty** p2, Ty* resP,  int nmass, 
    std::vector<ga_M > &ga1V, std::vector<ga_M > &ga2V,
    qlat::fft_desc_basic &fd, int clear=1, int invmode=1)
{
  TIMERA("meson_vectorEV");
  ///////check_prop_size(prop1);check_prop_size(prop2);
  int  NTt  = fd.Nv[3];
  long Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  qassert(ga1V.size() == ga2V.size());
  int Ngv = ga1V.size();
  if(clear == 1){zero_Ty(resP, Ngv*nmass*NTt*Nxyz , 1);}

  qlat::vector_acc<Ty > gMap;
  qlat::vector_acc<unsigned char > IMap;
  gMap.resize(Ngv*4*2);IMap.resize(Ngv*4*2);
  for(int iv=0;iv<Ngv;iv++){
    for(int i=0;i<4;i++){
      int j = iv*4 + i;
      gMap[0*Ngv*4+j] = ga1V[iv].g[i];
      gMap[1*Ngv*4+j] = ga2V[iv].g[i];
      IMap[0*Ngv*4+j] = ga1V[iv].ind[i];
      IMap[1*Ngv*4+j] = ga2V[iv].ind[i];
    }
  }

  Ty* gC_P = gMap.data();
  unsigned char*      gI_P = IMap.data();

  #if USEKERNEL==1
  std::vector<std::vector<char > > giEL;giEL.resize(Ngv);
  std::vector<std::vector<unsigned char   > > oiL ;oiL.resize(Ngv );

  ////reformulate index
  for(int iv=0;iv<Ngv;iv++){
  oiL[iv].resize(0);
  giEL[iv].resize(0);
  const int j1 = 0*Ngv*4 + iv*4 ;
  const int j2 = 1*Ngv*4 + iv*4 ;
  const Ty* gC1 = &(gC_P[j1]);
  const Ty* gC2 = &(gC_P[j2]);
  const unsigned char* gI1 = &(gI_P[j1]);
  const unsigned char* gI2 = &(gI_P[j2]);
  for(int d2=0;d2<4;d2++)
  for(int c2=0;c2<3;c2++)
  for(int d1=0;d1<4;d1++)
  for(int c1=0;c1<3;c1++)
  {
    const char off1 = (d2*3+c2)*12+gI1[d1]*3+c1;
    const char off2 = (gI2[d2]*3+c2)*12+d1*3+c1;
    const Ty g_tem = gC2[d2]*gC1[d1];
    const double norm = qlat::qnorm(g_tem);
    if(norm < 1e-20)continue;

    oiL[iv].push_back(off1);
    oiL[iv].push_back(off2);
    giEL[iv].push_back(char(g_tem.real()));
    giEL[iv].push_back(char(g_tem.imag()));
  }
  }

  std::vector<qlat::vector_gpu<char > > giEG;giEG.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){giEG[iv].copy_from(giEL[iv]);}
  qlat::vector_acc<char* > gP = EigenM_to_pointers(giEG);
  char** gPP = gP.data();

  std::vector<qlat::vector_gpu<unsigned char   > > oiG ; oiG.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){oiG[iv].copy_from(oiL[iv]);}
  qlat::vector_acc<unsigned char* > oP = EigenM_to_pointers(oiG);
  unsigned char** oPP = oP.data();

  qlat::vector_acc<int > iv_size;iv_size.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){iv_size[iv] = giEL[iv].size()/2;}
  int*  ivP = iv_size.data();

  //////long Ntotal  = nmass*NTt*Nxyz;
  //int mode = 0;mode = invmode*2 + clear;
  //const int BFACG = BFACG_SHARED;


  #if USEGLOBAL==1
  const int BFACG = BFACG_SHARED;
  #else
  const int BFACG = 1;
  #endif

  if(invmode==0)meson_vectorEV_kernel<Ty,0, BFACG>(p1, p2, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  if(invmode==1)meson_vectorEV_kernel<Ty,1, BFACG>(p1, p2, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  //if(mode==2)meson_vectorEV_kernel<Ty,1,0, BFACG>(p1, p2, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  //if(mode==3)meson_vectorEV_kernel<Ty,1,1, BFACG>(p1, p2, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  qacc_barrier(dummy);
  #endif

  #if USEKERNEL==0
  for(int iv=0;iv<Ngv;iv++){
  int j1 = 0*Ngv*4 + iv*4 ;
  int j2 = 1*Ngv*4 + iv*4 ;
  Ty* gC1 = &(gC_P[j1]);
  Ty* gC2 = &(gC_P[j2]);
  unsigned char* gI1 = &(gI_P[j1]);
  unsigned char* gI2 = &(gI_P[j2]);
  long offR = iv*nmass*NTt * Nxyz;
  for(int d2=0;d2<4;d2++)
  for(int c2=0;c2<3;c2++)
  for(int d1=0;d1<4;d1++)
  for(int c1=0;c1<3;c1++)
  {
  #pragma omp parallel for
  for(int ji=0;ji<nmass*NTt;ji++)
  {
    int massi = ji/NTt;
    int ti    = ji%NTt;

    int off1 = massi*12*12 + (d2*3+c2)*12+gI1[d1]*3+c1;
    int off2 = massi*12*12 + (gI2[d2]*3+c2)*12+d1*3+c1;

    Ty g_tem = gC2[d2]*gC1[d1];

    Ty* tp1 = p1[off1*NTt+ti];
    Ty* tp2 = p2[off2*NTt+ti];
    Ty* tr0 = &(resP[offR + (massi*NTt + ti)*Nxyz]);

    #if USEQACC==1
    if(invmode == 1){qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*qlat::qconj(tp2[i]) * g_tem);});}
    if(invmode == 0){qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*           (tp2[i]) * g_tem);});}
    #else
    Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vp1(tp1,Nxyz);
    Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vp2(tp2,Nxyz);
    Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vr0(tr0,Nxyz);
    if(invmode == 1)vr0 += (vp1*vp2.conjugate() * g_tem);
    if(invmode == 0)vr0 += (vp1*vp2             * g_tem);
    #endif

  }
  qacc_barrier(dummy);
  }
  }
  #endif

}

void meson_vectorEV(EigenM &prop1, EigenM &prop2, EigenV &res, std::vector<ga_M > &ga1V, std::vector<ga_M > &ga2V,
        qlat::fft_desc_basic &fd, int clear=1, int invmode=1)
{
  check_prop_size(prop1);check_prop_size(prop2);
  int  nmass = prop1.size()/(12*12*fd.Nv[3]);
  if(nmass == 0){res.resize(0);return;}
  int Ngv = ga1V.size();
  long resL = Ngv * nmass * fd.Nv[0]*fd.Nv[1]*fd.Nv[2] * fd.Nv[3];
  if(clear == 1){if(res.size()!= resL){res.resize(resL);}}

  if(res.size() != resL){print0("Size of res wrong. \n");qassert(false);}
  qassert(prop1.size() == prop2.size());

  qlat::vector_acc<Complexq* > prop1P = EigenM_to_pointers(prop1);
  qlat::vector_acc<Complexq* > prop2P = EigenM_to_pointers(prop2);

  Complexq** p1 = prop1P.data();
  Complexq** p2 = prop2P.data();
  Complexq* resP = res.data();
  meson_vectorEV(p1, p2, resP, nmass, ga1V, ga2V, fd, clear, invmode);
}

#ifdef QLAT_USE_ACC
template<typename Ty, int bfac, int Blocks>
__global__ void baryon_vectorEV_global(Ty** p1, Ty** p2, Ty** p3, Ty* resP,
  char** gPP, unsigned char** oPP, const int* ivP,
  const int nmass, const int NTt, const long Nxyz, const int Ngv)
{
  //unsigned long gi =  blockIdx.x*blockDim.x + threadIdx.x;
  const unsigned long gi =  blockIdx.x;
  //const unsigned int tid = threadIdx.x;
  const unsigned int tid = threadIdx.y*blockDim.x+ threadIdx.x;
  /////const unsigned int Tid = blockDim.x;
  const long Ntotal = nmass * NTt * Nxyz;
  const long Nbfac  = Ntotal/bfac;
  __shared__ Ty P1[bfac*12*12];
  __shared__ Ty P2[bfac*12*12];
  __shared__ Ty P3[bfac*12*12];

  if(gi*bfac < Ntotal){

  //__shared__ Ty buf[bfac*16+1];
  ////const long offR0 = gi;
  int bi0= 0;int dc = 0;
  int ji = 0;int massi = 0;int ti = 0;
  ////const long ixyz = (bi0*Nbfac + gi)%Nxyz;

  int jobN = bfac*12*12;
  unsigned int off = tid;
  while(off < jobN){
    bi0= off/(12*12);
    dc = off%(12*12);
    ji    = (bi0*Nbfac + gi)/Nxyz;
    massi = ji/NTt;
    ti    = ji%NTt;
    long ixyz = (bi0*Nbfac + gi)%Nxyz;
    //massi = (ji + bi)/NTt;
    //ti    = (ji + bi)%NTt;
    P1[dc*bfac + bi0] = p1[(massi*12*12 + dc)*NTt + ti][ixyz];
    P2[dc*bfac + bi0] = p2[(massi*12*12 + dc)*NTt + ti][ixyz];
    P3[dc*bfac + bi0] = p3[(massi*12*12 + dc)*NTt + ti][ixyz];
    off += Blocks;
  }
  __syncthreads();

  int ini = 0;
  int dv = 0;
  unsigned int MAX = 0;

  const int bfacC = bfac;
  const int Nth   = Blocks/bfac;
  const unsigned int Each  = 16*bfacC;
  const unsigned int GROUP = (Blocks/bfac)*Each;
  unsigned char* s0 = NULL;
           char* s1 = NULL;

  const int bi =  threadIdx.y;
  const int ai =  threadIdx.x;

  const int ba = (threadIdx.y/bfacC)*bfacC + 0;
  const int aa = (threadIdx.y%bfacC)*blockDim.x + threadIdx.x;

  __shared__ Ty buf[bfacC*Blocks];
  __shared__ unsigned char pos[3*GROUP];
  __shared__ char g0[2*GROUP];

  for(int iv=0;iv<Ngv;iv++)
  {
    for(int bz=0;bz<bfacC;bz++){buf[bz*Blocks + tid] = 0;}
    MAX = ivP[iv];
    jobN = (MAX + GROUP -1 )/GROUP;
    ini = 0; dv = GROUP;
    for(int ji=0;ji<jobN;ji++){
      if(ini + dv >= MAX){dv = MAX - ini;}
      s0 = &(oPP[iv][ini*3]);
      s1 = &(gPP[iv][ini*2]);

      off = tid;
      while(off < dv*3){pos[off] = s0[off];off += Blocks;}
      off = tid;
      while(off < dv*2){g0[off]  = s1[off];off += Blocks;}
      __syncthreads();

      off = aa;
      while(off < dv){
        const Ty* t1 = &P1[(pos[off*3+0])*bfac + ba];
        const Ty* t2 = &P2[(pos[off*3+1])*bfac + ba];
        const Ty* t3 = &P3[(pos[off*3+2])*bfac + ba];
        Ty gtem = Ty(g0[off*2+0], g0[off*2+1]);
        if(bfacC == 1){
          buf[aa*bfac+ba] += (t1[0] * t2[0] * t3[0]*gtem);
        }else{
          Ty* b0 = &buf[aa*bfac+ba];
          for(int z=0;z<bfacC;z++){b0[z] += (t1[z] * t2[z] * t3[z]*gtem);}
        }
        off += Nth*bfacC;
      }
      __syncthreads();

      ini += dv;
    }

    for(int atem=1;atem<bfacC;atem++){buf[(0*Nth+ai)*bfac+bi] += buf[(atem*Nth+ai)*bfac+bi];} __syncthreads();

    if(Nth >=256){if(ai <128){buf[ai*bfac + bi] += buf[(ai+128)*bfac + bi];}__syncthreads();}
    if(Nth >=128){if(ai < 64){buf[ai*bfac + bi] += buf[(ai+ 64)*bfac + bi];}__syncthreads();}
    if(Nth >= 64){if(ai < 32){buf[ai*bfac + bi] += buf[(ai+ 32)*bfac + bi];}__syncthreads();}
    if(Nth >= 32){if(ai < 16){buf[ai*bfac + bi] += buf[(ai+ 16)*bfac + bi];}__syncthreads();}
    if(Nth >= 16){if(ai <  8){buf[ai*bfac + bi] += buf[(ai+  8)*bfac + bi];}__syncthreads();}
    if(Nth >=  8){if(ai <  4){buf[ai*bfac + bi] += buf[(ai+  4)*bfac + bi];}__syncthreads();}
    if(Nth >=  4){if(ai <  2){buf[ai*bfac + bi] += buf[(ai+  2)*bfac + bi];}__syncthreads();}

    if(ai == 0){
      //if(clear == 0){resP[iv*Ntotal + bi*Nbfac + gi] += (buf[bi] + buf[bfac+bi]);}
      //if(clear == 1){resP[iv*Ntotal + bi*Nbfac + gi]  = (buf[bi] + buf[bfac+bi]);}
      resP[iv*Ntotal + bi*Nbfac + gi] += (buf[bi] + buf[bfac+bi]); 
    }
    __syncthreads();

  }

  }


}
#endif

template<typename Ty, int bfac>
void baryon_vectorEV_kernel(Ty** p1, Ty** p2, Ty** p3, Ty* resP,
  char** gPP, unsigned char** oPP, const int* ivP,
  const int nmass, const int NTt, const long Nxyz, const int Ngv)
{
  long Ntotal  = nmass*NTt*Nxyz;
  ////print0("nmass %d, NTt %d, Nxyz %d \n", int(nmass), int(NTt), int(Nxyz));
  if(Ntotal % bfac != 0){abort_r("Please correct your bfac! \n");}
  long Nbfac = Ntotal/bfac;

  #if USEGLOBAL==1
  const int nt = 16;
  const int Blocks = nt*bfac;
  dim3 dimBlock(    nt, bfac, 1);
  dim3 dimGrid(  Nbfac,  1, 1);
  baryon_vectorEV_global<Complexq, bfac, Blocks><<<dimGrid, dimBlock>>>(p1, p2, p3, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  qacc_barrier(dummy);
  #else
  if((nmass*NTt) % bfac != 0){abort_r("Please correct your bfac! \n");}
  qacc_for(gi, Nbfac ,
  {
    Ty buf[bfac+1];
    Ty P1[bfac*12*12+1];
    Ty P2[bfac*12*12+1];
    Ty P3[bfac*12*12+1];

    long ixyz = gi%Nxyz;
    int ji    = (gi/Nxyz)*bfac + 0;
    int massi = ji/NTt;
    int ti    = ji%NTt;
    const long offR0 = (massi*NTt + ti)*Nxyz + ixyz;

    for(int bi=0;bi<bfac;bi++)
    {
      massi = (ji+bi)/NTt;
      ti    = (ji+bi)%NTt;

      for(int dc=0;dc<12*12;dc++){
        P1[dc*bfac + bi] = p1[(massi*12*12 + dc)*NTt + ti][ixyz];
        P2[dc*bfac + bi] = p2[(massi*12*12 + dc)*NTt + ti][ixyz];
        P3[dc*bfac + bi] = p3[(massi*12*12 + dc)*NTt + ti][ixyz];
      }
    }

    for(int iv=0;iv<Ngv;iv++)
    {
      for(int bi=0;bi<bfac;bi++){buf[bi] = 0;}

      for(int off=0;off<ivP[iv];off++)
      {
        const Ty* t1 = &P1[(oPP[iv][off*3+0])*bfac];
        const Ty* t2 = &P2[(oPP[iv][off*3+1])*bfac];
        const Ty* t3 = &P3[(oPP[iv][off*3+2])*bfac];
        const Ty gtem = Ty(gPP[iv][off*2+0], gPP[iv][off*2+1]);
        for(int bi=0;bi<bfac;bi++)
        {
          buf[bi] += (t1[bi] * t2[bi] * t3[bi] * gtem);
        }
      }

      long offR = iv * Ntotal;
      Ty* r0 = &resP[offR + offR0];
      for(int bi=0;bi<bfac; bi++){
        r0[bi*Nxyz]  += buf[bi];
        //if(clear == 0){r0[bi*Nxyz] += buf[bi];}
        //if(clear == 1){r0[bi*Nxyz]  = buf[bi];}
      }
    }

  });
  #endif


}


/////A source gamma, B sink Gamma, G projections with fermion sign, mL shape of diagram
template <typename Ty>
void baryon_vectorEV(Ty** p1, Ty** p2, Ty** p3, Ty* resP, int nmass,
  ga_M &A, ga_M &B, qlat::vector_acc<Ty > &GV, 
  qlat::vector_acc<int > &mLV, fft_desc_basic &fd, int clear=1)
{
  TIMER("Proton_vectorEV");
  int NTt  = fd.Nv[3];
  long Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int Ngv = GV.size()/16;
  qassert(GV.size()  == 16*Ngv);
  qassert(mLV.size() == 3*Ngv);

  if(clear == 1){zero_Ty(resP, Ngv*nmass*NTt*Nxyz , 1);}

  qlat::vector_acc<Ftype > epslV;epslV.resize(9);
  for(int i=0;i<3;i++){epslV[i*3+i]=0;epslV[i*3 + (i+1)%3]=1;epslV[i*3 + (i+2)%3]=-1;}
  qlat::vector_acc<Ty > gMap;
  qlat::vector_acc<int > IMap;
  gMap.resize(4*2);IMap.resize(4*2);
  for(int i=0;i<4;i++){
    /////int j = + i;
    gMap[0*4+i] = A.g[i];
    gMap[1*4+i] = B.g[i];
    IMap[0*4+i] = A.ind[i];
    IMap[1*4+i] = B.ind[i];
  }

  const Ftype* epsl = epslV.data();
  const Ty* gCA = &((gMap.data())[0*4]);
  const Ty* gCB = &((gMap.data())[1*4]);
  const int* gIA = &((IMap.data())[0*4]);
  const int* gIB = &((IMap.data())[1*4]);
  const Ty* GVP = GV.data();
  const int*  mLP     = mLV.data();

  /////contraction Kernel
  #if USEKERNEL==1
  ////long Ntotal  = nmass*NTt*Nxyz;
  /////const int Loff = 3*3*3*3*4*4*4*4;
  std::vector<std::vector<char > > giEL;giEL.resize(Ngv);//giEL.resize(  Ngv*Loff);
  std::vector<std::vector<unsigned char   > > oiL ;oiL.resize(Ngv );//oiL.resize(3*Ngv*Loff);
  int bmL[3];
  int nmL[3];
  for(int iv=0;iv<Ngv;iv++)
  {
    oiL[iv].resize(0);
    giEL[iv].resize(0);

    const Ty* G  = &GVP[iv*16];
    const int*      mL = &mLP[iv*3];

    for(int a1=0;a1<3;a1++)
    for(int ia=1;ia<3;ia++)
    for(int b1=0;b1<3;b1++)
    for(int ib=1;ib<3;ib++)
    {
      int b2=(b1+ib)%3,b3=(b2+ib)%3;
      int a2=(a1+ia)%3,a3=(a2+ia)%3;
      for(int m2=0;m2<4;m2++)
      for(int m1=0;m1<4;m1++)
      for(int n2=0;n2<4;n2++)
      for(int n1=0;n1<4;n1++)
      {
        const Ty Gtem =  G[m1*4+n1];
        const double norm = qlat::qnorm(Gtem);
        if(norm < 1e-20)continue;

        const int m3 = gIA[m2];
        const int n3 = gIB[n2];
        const Ty giE = epsl[a1*3 + a2]*epsl[b1*3 + b2]*gCA[m2]*gCB[n2]*G[m1*4+n1];
        nmL[0] = n1;nmL[1] = n2;nmL[2] = n3;
        bmL[0] = b1;bmL[1] = b2;bmL[2] = b3;
        const int nm1 = nmL[mL[0]];
        const int nm2 = nmL[mL[1]];
        const int nm3 = nmL[mL[2]];

        const int bm1 = bmL[mL[0]];
        const int bm2 = bmL[mL[1]];
        const int bm3 = bmL[mL[2]];

        const int o1 = (m1*3+a1)*12+(nm1*3+bm1);
        const int o2 = (m2*3+a2)*12+(nm2*3+bm2);
        const int o3 = (m3*3+a3)*12+(nm3*3+bm3);

        ////buf += (P1[o1] * P2[o2] *P3[o3] * giE);
        oiL[iv].push_back(o1);
        oiL[iv].push_back(o2);
        oiL[iv].push_back(o3);
        giEL[iv].push_back(char(giE.real()));
        giEL[iv].push_back(char(giE.imag()));
        //giEL[iv].push_back(giE);

      }
    }
  }

  std::vector<qlat::vector_gpu<char > > giEG;giEG.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){giEG[iv].copy_from(giEL[iv]);}
  qlat::vector_acc<char* > gP = EigenM_to_pointers(giEG);
  char** gPP = gP.data();

  std::vector<qlat::vector_gpu<unsigned char   > > oiG ; oiG.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){oiG[iv].copy_from(oiL[iv]);}
  qlat::vector_acc<unsigned char* > oP = EigenM_to_pointers(oiG);
  unsigned char** oPP = oP.data();

  qlat::vector_acc<int > iv_size;iv_size.resize(Ngv);
  for(int iv=0;iv<Ngv;iv++){iv_size[iv] = giEL[iv].size()/2;}
  int*  ivP = iv_size.data();
  ///int maxNv = iv_size[0];
  ///for(int iv=0;iv<Ngv;iv++){if(iv_size[iv] > maxNv){maxNv = iv_size[iv];}}

  //{
  //long total = 0;
  //for(int iv=0;iv<Ngv;iv++){total += iv_size[iv];}
  //print0("==Ngv %d, total %d \n", int(Ngv), int(total));
  //}

  ////int mode = 0;mode = clear;
  const int BFACG = BFACG_SHARED;
  //if(mode==0)baryon_vectorEV_kernel<Complexq, 0, BFACG>(p1, p2, p3, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  //if(mode==1)baryon_vectorEV_kernel<Complexq, 1, BFACG>(p1, p2, p3, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);
  baryon_vectorEV_kernel<Complexq, BFACG>(p1, p2, p3, resP, gPP, oPP, ivP, nmass, NTt, Nxyz, Ngv);

  #endif

  #if USEKERNEL==0
  for(int iv=0;iv<Ngv;iv++)
  {
    long offR = iv*nmass*NTt * Nxyz;
    const Ty* G  = &(GVP[iv*16 + 0]);
    const int*      mL = &(mLP[iv*3 + 0]);
    int bmL[3];
    int nmL[3];

    for(int a1=0;a1<3;a1++)
    for(int ia=1;ia<3;ia++)
    for(int b1=0;b1<3;b1++)
    for(int ib=1;ib<3;ib++)
    {
      int b2=(b1+ib)%3,b3=(b2+ib)%3;
      int a2=(a1+ia)%3,a3=(a2+ia)%3;
      for(int m2=0;m2<4;m2++)
      for(int m1=0;m1<4;m1++)
      for(int n2=0;n2<4;n2++)
      for(int n1=0;n1<4;n1++)
      {
        Ty Gtem =  G[m1*4+n1];
        double norm = qlat::qnorm(Gtem);
        if(norm < 1e-20)continue;

        int m3 = gIA[m2];
        int n3 = gIB[n2];
        Ty giE = epsl[a1*3 + a2]*epsl[b1*3 + b2]*gCA[m2]*gCB[n2]*G[m1*4+n1];
        nmL[0] = n1;nmL[1] = n2;nmL[2] = n3;
        bmL[0] = b1;bmL[1] = b2;bmL[2] = b3;
        int nm1 = nmL[mL[0]];
        int nm2 = nmL[mL[1]];
        int nm3 = nmL[mL[2]];

        int bm1 = bmL[mL[0]];
        int bm2 = bmL[mL[1]];
        int bm3 = bmL[mL[2]];

        #pragma omp parallel for
        for(int ji=0;ji<nmass*NTt;ji++)
        {
          int massi = ji/NTt;
          int ti    = ji%NTt;

          int o1 = massi*12*12 + (m1*3+a1)*12+(nm1*3+bm1);
          int o2 = massi*12*12 + (m2*3+a2)*12+(nm2*3+bm2);
          int o3 = massi*12*12 + (m3*3+a3)*12+(nm3*3+bm3);

          Ty* tp1 = p1[o1*NTt+ti];
          Ty* tp2 = p2[o2*NTt+ti];
          Ty* tp3 = p3[o3*NTt+ti];
          Ty* tr0 = &(resP[offR + (massi*NTt + ti)*Nxyz]);

          #if USEQACC==1
          qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*tp2[i]*tp3[i] * giE); });
          #else
          Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vp1(tp1,Nxyz);
          Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vp2(tp2,Nxyz);
          Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vp3(tp3,Nxyz);
          Eigen::Map<Eigen::Array<Ty ,Eigen::Dynamic,1 > > vr0(tr0,Nxyz);
          vr0 += (vp1*vp2*vp3 * giE);
          #endif

        }
        qacc_barrier(dummy);
      }
    }
  }
  #endif

}

void baryon_vectorEV(EigenM &prop1, EigenM &prop2, EigenM &prop3, EigenV &res,
  ga_M &A, ga_M &B, qlat::vector_acc<Complexq > &GV, qlat::vector_acc<int > &mLV,
  fft_desc_basic &fd,int clear=1)
{
  int NTt  = fd.Nv[3];
  long Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];

  check_prop_size(prop1);check_prop_size(prop2);check_prop_size(prop3);
  int nmass = prop1.size()/(12*12*NTt);
  qassert(prop1.size() == prop2.size());
  qassert(prop1.size() == prop3.size());
  int Ngv = GV.size()/16;
  long resL = Ngv * nmass*NTt * Nxyz;
  if(clear == 1){if(res.size()!= resL){res.resize(resL); } }
  if(res.size() != resL){print0("Size of res wrong. \n");qassert(false);}

  qlat::vector_acc<Complexq* > prop1P = EigenM_to_pointers(prop1);
  qlat::vector_acc<Complexq* > prop2P = EigenM_to_pointers(prop2);
  qlat::vector_acc<Complexq* > prop3P = EigenM_to_pointers(prop3);
  Complexq** p1 = prop1P.data();
  Complexq** p2 = prop2P.data();
  Complexq** p3 = prop3P.data();
  Complexq* resP = res.data();

  baryon_vectorEV(p1, p2, p3, resP, nmass, A, B, GV, mLV, fd, clear);
}


std::vector<int >  get_sec_map(int dT,int nt)
{
  std::vector<int > map_sec;map_sec.resize(nt);
  int secN = 2*nt/dT;double lensec = nt/(1.0*secN);
  int tcount = 0;
  int t0 = 0;
  for(int si=0;si<secN;si++)
  {
    for(int t=t0;t <= (si+1)*lensec;t++)
    {
      map_sec[t] = si;
      tcount = tcount + 1;
    }
    t0 = tcount;
  }
  return map_sec;

}

void proton_vectorE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
        EigenV &res, fft_desc_basic &fd, ga_M &ga1,int t0,int dT,int clear=1,int oppo=0)
{
  TIMER("Proton_vectorE");
  int NTt  = fd.Nv[3];
  LInt Nxyz = fd.Nv[0]*fd.Nv[1]*fd.Nv[2];
  int nmass = prop1.size()/(12*12*NTt);
  qassert(prop1.size() == prop2.size());
  qassert(prop1.size() == prop3.size());

  if(clear == 1){ini_resE(res,nmass,fd);}

  //int nv = res.size();int Nsize = res[0].size();
  EigenV resE0;resE0.resize(res.size());
  EigenV resE1;resE1.resize(res.size());
  //for(int i=0;i<nv;i++)
  //{
  //  resE0[i].resize(Nsize);
  //  resE1[i].resize(Nsize);
  //}

  proton_vectorE(prop1,prop2,prop3,ga1,0,ga1,0,resE0,fd,1);
  proton_vectorE(prop1,prop2,prop3,ga1,1,ga1,1,resE0,fd,0);
  proton_vectorE(prop1,prop2,prop3,ga1,2,ga1,2,resE1,fd,1);
  proton_vectorE(prop1,prop2,prop3,ga1,3,ga1,3,resE1,fd,0);

  std::vector<int > map_sec = get_sec_map(dT,fd.nt);
  //////int Nt = fd.Nt;

  /////int t0 = 0;
  int nt = fd.nt;
  ///for(int massi=0;massi<nmass;massi++)
  ///for(int ti = 0;ti<Nt;ti++)
  #pragma omp parallel for
  for(int ji=0;ji<nmass*NTt;ji++)
  {
    int massi = ji/NTt;
    int ti    = ji%NTt;
    int t = ti + fd.Pos0[fd.rank][3];
    Complexq* tr0 = &(res.data()[(massi*NTt+ti)*Nxyz]);
    Complexq* tv0 = &(resE0.data()[(massi*NTt+ti)*Nxyz]);
    Complexq* tv1 = &(resE1.data()[(massi*NTt+ti)*Nxyz]);

    #if USEQACC==0
    EA r0(tr0,Nxyz);
    EA v0(tv0,Nxyz);
    EA v1(tv1,Nxyz);
    #endif

    //EA r0(&res[massi*Nt+ti][0],Nxyz);
    //EA v0(&resE0[massi*Nt+ti][0],Nxyz);
    //EA v1(&resE1[massi*Nt+ti][0],Nxyz);

    if(map_sec[(t-t0+nt)%nt]%2==0)
    {
      #if USEQACC==1
      if(oppo==0)qacc_forNB(i, long(Nxyz), { tr0[i] += tv0[i]; });
      if(oppo==1)qacc_forNB(i, long(Nxyz), { tr0[i] += tv1[i]; });
      #else
      if(oppo==0){r0 += v0;}
      if(oppo==1){r0 += v1;}
      #endif

    }
    if(map_sec[(t-t0+nt)%nt]%2==1)
    {
      #if USEQACC==1
      if(oppo==0)qacc_forNB(i, long(Nxyz), { tr0[i] += tv1[i]; });
      if(oppo==1)qacc_forNB(i, long(Nxyz), { tr0[i] += tv0[i]; });
      #else
      if(oppo==0){r0 += v1;}
      if(oppo==1){r0 += v0;}
      #endif

    }
  }
  qacc_barrier(dummy);

}

void proton_corrE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
   ga_M &ga2,int ind2,ga_M &ga1,int ind1,
  EigenV &res, fft_desc_basic &fd,int clear=1,int imom=50505)
{
  ///int NTt  = fd.Nv[3];
  ////LInt Nxyz = prop1[0].size();
  ///int nmass = prop1.size()/(12*12*NTt);
  ////int nt = fd.nt;

  EigenV resE;
  ////ini_resE(resE,nmass,fd);

  proton_vectorE(prop1,prop2,prop3,ga2,ind2,ga1,ind1,resE,fd,1);

  vec_corrE(resE,res,fd,clear,imom);
}

void proton_corrE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
 EigenV &res, fft_desc_basic &fd, ga_M &ga1,int t0,int dT,int clear=1,int imom=505050)
{
  ///int NTt  = fd.Nv[3];
  ////LInt Nxyz = prop1[0].size();
  ///int nmass = prop1.size()/(12*12*NTt);
  ////int nt = fd.nt;

  EigenV resE;
  ////ini_resE(resE,nmass,fd);
  proton_vectorE(prop1,prop2,prop3,resE,fd, ga1, t0,dT,1);

  vec_corrE(resE,res,fd,clear,imom);
}


/////A source gamma, B sink Gamma, G projections with fermion sign, mL shape of diagram
void baryon_vectorE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
  ga_M &A, ga_M &B, qlat::vector_acc<Complexq > &G, qlat::vector_acc<int > &mL,
        EigenV &res, fft_desc_basic &fd,int clear=1)
{
  TIMER("Proton_vectorE");
  check_prop_size(prop1);check_prop_size(prop2);check_prop_size(prop3);
  int NTt  = fd.Nv[3];
  LInt Nxyz = prop1[0].size();
  int nmass = prop1.size()/(12*12*NTt);
  qassert(prop1.size() == prop2.size());
  qassert(prop1.size() == prop3.size());
  qassert(G.size()  == 16);
  qassert(mL.size() == 3);
  if(clear == 1){ini_resE(res,nmass,fd);}

  //if(res.size()%NTt !=0 or res.size()==0){print0("Size of res wrong. \n");qassert(false);}
  if(res.size()==0){print0("Size of res wrong. \n");qassert(false);}

  Ftype epsl[3][3];
  for(int i=0;i<3;i++){epsl[i][i]=0;epsl[i][(i+1)%3]=1;epsl[i][(i+2)%3]=-1;}

  //mL = {};
  //std::vector<int > mL;mL.resize(3);
  //mL[0] = 0;mL[1] = 1;mL[2] = 2;
  std::vector<int > nmL;nmL.resize(3);
  std::vector<int > bmL;bmL.resize(3);

  {
    for(int a1=0;a1<3;a1++)
    for(int ia=1;ia<3;ia++)
    for(int b1=0;b1<3;b1++)
    for(int ib=1;ib<3;ib++)
    {
      int b2=(b1+ib)%3,b3=(b2+ib)%3;
      int a2=(a1+ia)%3,a3=(a2+ia)%3;
      for(int m2=0;m2<4;m2++)
      for(int m1=0;m1<4;m1++)
      for(int n2=0;n2<4;n2++)
      for(int n1=0;n1<4;n1++)
      {
        Complexq Gv =  G[m1*4+n1];
        double norm = std::sqrt(Gv.real()*Gv.real() + Gv.imag()*Gv.imag());
        if(norm < 1e-20)continue;

        int m3 = A.ind[m2];
        int n3 = B.ind[n2];
        Complexq giE = epsl[a1][a2]*epsl[b1][b2]*A.g[m2]*B.g[n2]*G[m1*4+n1];
        nmL[0] = n1;nmL[1] = n2;nmL[2] = n3;
        bmL[0] = b1;bmL[1] = b2;bmL[2] = b3;
        int nm1 = nmL[mL[0]];
        int nm2 = nmL[mL[1]];
        int nm3 = nmL[mL[2]];

        int bm1 = bmL[mL[0]];
        int bm2 = bmL[mL[1]];
        int bm3 = bmL[mL[2]];

        #pragma omp parallel for
        for(int ji=0;ji<nmass*NTt;ji++)
        {
          int massi = ji/NTt;
          int ti    = ji%NTt;

          int o1 = massi*12*12 + (m1*3+a1)*12+(nm1*3+bm1);
          int o2 = massi*12*12 + (m2*3+a2)*12+(nm2*3+bm2);
          int o3 = massi*12*12 + (m3*3+a3)*12+(nm3*3+bm3);

          Complexq* tp1 = prop1[o1*NTt+ti].data();
          Complexq* tp2 = prop2[o2*NTt+ti].data();
          Complexq* tp3 = prop3[o3*NTt+ti].data();
          Complexq* tr0 = &(res.data()[(massi*NTt + ti)*Nxyz]);

          #if USEQACC==1
          qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*tp2[i]*tp3[i] * giE); });
          #else
          EA vp1(tp1,Nxyz);
          EA vp2(tp2,Nxyz);
          EA vp3(tp3,Nxyz);
          EA vr0(tr0,Nxyz);
          vr0 += (vp1*vp2*vp3 * giE);
          #endif

          //EA vp1(&prop1[o1*NTt+ti][0],Nxyz);
          //EA vp2(&prop2[o2*NTt+ti][0],Nxyz);
          //EA vp3(&prop3[o3*NTt+ti][0],Nxyz);
          //EA vr0(&res[massi*NTt + ti][0],Nxyz);

        }
        qacc_barrier(dummy);
      }
    }
  }

}


////A source gamma, B sink Gamma, G projections with fermion sign, mL shape of diagram
void baryon_vectorE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
  ga_M &A, ga_M &B, qlat::vector_acc<Complexq > &G, qlat::vector_acc<int > &mL, int insertion,
        EigenM &resP, fft_desc_basic &fd,int clear=1)
{
  TIMER("Proton_vectorE");
  check_prop_size(prop1);check_prop_size(prop2);check_prop_size(prop3);
  int NTt  = fd.Nv[3];
  LInt Nxyz = prop1[0].size();
  int nmass = prop1.size()/(12*12*NTt);
  qassert(prop1.size() == prop2.size());
  qassert(prop1.size() == prop3.size());
  qassert(G.size()  == 16);
  qassert(mL.size() == 3);
  qassert(fd.order_ch == 0);

  if(clear==1){ini_propE(resP,nmass,fd);}
  /////check_prop_size(resP);

  Ftype epsl[3][3];
  for(int i=0;i<3;i++){epsl[i][i]=0;epsl[i][(i+1)%3]=1;epsl[i][(i+2)%3]=-1;}

  //mL = {};
  //std::vector<int > mL;mL.resize(3);
  //mL[0] = 0;mL[1] = 1;mL[2] = 2;
  std::vector<int > nmL;nmL.resize(3);
  std::vector<int > bmL;bmL.resize(3);

  {
    for(int a1=0;a1<3;a1++)
    for(int ia=1;ia<3;ia++)
    for(int b1=0;b1<3;b1++)
    for(int ib=1;ib<3;ib++)
    {
      int b2=(b1+ib)%3,b3=(b2+ib)%3;
      int a2=(a1+ia)%3,a3=(a2+ia)%3;
      for(int m2=0;m2<4;m2++)
      for(int m1=0;m1<4;m1++)
      for(int n2=0;n2<4;n2++)
      for(int n1=0;n1<4;n1++)
      {
        Complexq Gv =  G[m1*4+n1];
        double norm = std::sqrt(Gv.real()*Gv.real() + Gv.imag()*Gv.imag());
        if(norm < 1e-20)continue;

        int m3 = A.ind[m2];
        int n3 = B.ind[n2];
        Complexq giE = epsl[a1][a2]*epsl[b1][b2]*A.g[m2]*B.g[n2]*G[m1*4+n1];
        nmL[0] = n1;nmL[1] = n2;nmL[2] = n3;
        bmL[0] = b1;bmL[1] = b2;bmL[2] = b3;
        int nm1 = nmL[mL[0]];
        int nm2 = nmL[mL[1]];
        int nm3 = nmL[mL[2]];

        int bm1 = bmL[mL[0]];
        int bm2 = bmL[mL[1]];
        int bm3 = bmL[mL[2]];

        #pragma omp parallel for
        for(int ji=0;ji<nmass*NTt;ji++)
        {
          int massi = ji/NTt;
          int ti    = ji%NTt;

          int o1 = massi*12*12 + (m1*3+a1)*12+(nm1*3+bm1);
          int o2 = massi*12*12 + (m2*3+a2)*12+(nm2*3+bm2);
          int o3 = massi*12*12 + (m3*3+a3)*12+(nm3*3+bm3);

          int r0 = 0;
          if(insertion == 0){r0 = massi*12*12 + (m1*3+a1)*12+(nm1*3+bm1);}
          if(insertion == 1){r0 = massi*12*12 + (m2*3+a2)*12+(nm2*3+bm2);}
          if(insertion == 2){r0 = massi*12*12 + (m3*3+a3)*12+(nm3*3+bm3);}

          Complexq* tp1 = prop1[o1*NTt+ti].data();
          Complexq* tp2 = prop2[o2*NTt+ti].data();
          Complexq* tp3 = prop3[o3*NTt+ti].data();
          Complexq* tr0 = resP[r0*NTt+ti].data();

          #if USEQACC==1
          if(insertion == 0)qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp2[i]*tp3[i] * giE); });
          if(insertion == 1)qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*tp3[i] * giE); });
          if(insertion == 2)qacc_forNB(i, long(Nxyz),{ tr0[i] += (tp1[i]*tp2[i] * giE); });
          #else
          EA vp1(tp1,Nxyz);
          EA vp2(tp2,Nxyz);
          EA vp3(tp3,Nxyz);
          EA vr0(tr0,Nxyz);
          if(insertion == 0){vr0 += vp2*vp3 * giE;}
          if(insertion == 1){vr0 += vp1*vp3 * giE;}
          if(insertion == 2){vr0 += vp1*vp2 * giE;}
          #endif
          
          //EA vp1(&prop1[o1*NTt+ti][0],Nxyz);
          //EA vp2(&prop2[o2*NTt+ti][0],Nxyz);
          //EA vp3(&prop3[o3*NTt+ti][0],Nxyz);
          //EA vr0( &resP[r0*NTt+ti][0],Nxyz);

        }
        qacc_barrier(dummy);
      }
    }
  }

}

void baryon_corrE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
   ga_M &ga2,int ind2,ga_M &ga1,int ind1,
  EigenV &res, fft_desc_basic &fd,int clear=1,int imom=50505)
{
  int NTt  = fd.Nv[3];
  ////LInt Nxyz = prop1[0].size();
  int nmass = prop1.size()/(12*12*NTt);
  ////int nt = fd.nt;

  EigenV resE;
  ini_resE(resE,nmass,fd);

  qlat::vector_acc<Complexq > G;G.resize(16);
  qlat::vector_acc<int > mL;mL.resize(3);

  clear_qv(G);G[ind2*4 + ind1] = +1.0;
  mL[0] = 0;mL[1] = 1;mL[2] = 2;
  baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 1);
  clear_qv(G);G[ind2*4 + ind1] = -1.0;
  mL[0] = 1;mL[1] = 0;mL[2] = 2;
  baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////proton_vectorE(prop1,prop2,prop3,ga2,ind2,ga1,ind1,resE,fd,1);

  vec_corrE(resE,res,fd,clear,imom);
}


void Omega_corrE(EigenM &prop1, EigenM &prop2, EigenM &prop3,
   ga_M &ga2,int ind2,ga_M &ga1,int ind1,
  EigenV &res, fft_desc_basic &fd,int clear=1,int imom=50505)
{
  int NTt  = fd.Nv[3];
  ///LInt Nxyz = prop1[0].size();
  int nmass = prop1.size()/(12*12*NTt);
  ///int nt = fd.nt;

  EigenV resE;
  ini_resE(resE,nmass,fd);

  qlat::vector_acc<Complexq > G;G.resize(16);
  qlat::vector_acc<int > mL;mL.resize(3);

  std::vector<int > dia;dia.resize(6);
  std::vector<int > sn ;sn.resize(6);
  dia[0] = 9012;sn[0] =  1;
  dia[1] = 9102;sn[1] = -1;
  dia[2] = 9021;sn[2] = -1;
  dia[3] = 9201;sn[3] =  1;
  dia[4] = 9210;sn[4] = -1;
  dia[5] = 9120;sn[5] =  1;

  for(int di=0;di<6;di++)
  {
    clear_qv(G);G[ind2*4 + ind1] = sn[di];
    mL[0] = (dia[di]/100)%10;mL[1] =  (dia[di]%100)/10;mL[2] = dia[di]%10;
    baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);
  }
     
  ////clear_qv(G);G[ind2*4 + ind1] = +1.0;
  ////mL[0] = 0;mL[1] = 1;mL[2] = 2;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////clear_qv(G);G[ind2*4 + ind1] = -1.0;
  ////mL[0] = 1;mL[1] = 0;mL[2] = 2;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////clear_qv(G);G[ind2*4 + ind1] = -1.0;
  ////mL[0] = 0;mL[1] = 2;mL[2] = 1;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////clear_qv(G);G[ind2*4 + ind1] = +1.0;
  ////mL[0] = 2;mL[1] = 0;mL[2] = 1;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////clear_qv(G);G[ind2*4 + ind1] = -1.0;
  ////mL[0] = 2;mL[1] = 1;mL[2] = 0;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);

  ////clear_qv(G);G[ind2*4 + ind1] = +1.0;
  ////mL[0] = 1;mL[1] = 2;mL[2] = 0;
  ////baryon_vectorE(prop1,prop2,prop3, ga2,ga1, G, mL, resE, fd, 0);


  ////proton_vectorE(prop1,prop2,prop3,ga2,ind2,ga1,ind1,resE,fd,1);

  vec_corrE(resE,res,fd,clear,imom);
}

template <typename Ty>
void get_num_time(qlat::FieldM<Ty, 1>& noise,int &number_t, int &t_ini)
{
  qlat::Geometry& geo = noise.geo();
  qlat::vector_acc<int > nv,Nv,mv;
  geo_to_nv(geo, nv, Nv, mv);
  //int nx,ny,nz,nt;
  //nx = nv[0];ny = nv[1];nz = nv[2];nt = nv[3];
  int nt = nv[3];
  LInt Nsite = Nv[0]*Nv[1]*Nv[2]*Nv[3];

  std::vector<double > fullt(nt);for(int ti=0;ti<nt;ti++){fullt[ti]=0.0;}
  for(unsigned int isp=0; isp< Nsite; isp++)
  {
    ////position p = noise.desc->get_position(isp,get_node_rank());
    Coordinate xl0 = geo.coordinate_from_index(isp);
    Coordinate xg0 = geo.coordinate_g_from_l(xl0);
    {    
      ///auto tem_source = noise.data[isp];
      auto tem_source =  noise.get_elem(isp);
      if(qnorm(tem_source)>0.01)
      {    
        fullt[xg0[3]] = 1.0; 
      }    
    }    
  }
  sum_all_size((double* ) &fullt[0],nt);
  number_t = 0; 
  for(int ti=0;ti<nt;ti++){if(fullt[ti]>0.0)number_t += 1;}
  for(int ti=0;ti<nt;ti++){if(fullt[ti]>0.0){t_ini = ti;break;}}
}

void meson_corr_write(Propagator4d &propVa, Propagator4d &propVb, int pos, std::vector<double > &write, int offw, const Geometry &geo, int a=0, int b=0, int c=0 , int d=0)
{
  print_mem_info();
  fft_desc_basic fd(geo);
  //qlat::vector<int > nv, Nv, mv;
  //geo_to_nv(geo, nv, Nv, mv);
  int nt = fd.nt;

  ///char output[500];
  ///sprintf(output,   out_n.c_str());
  ///print0("output %s \n", output);

  EigenM propa,propb;
  copy_propE(propVa, propa, fd );
  copy_propE(propVb, propb, fd );

  ///Coordinate xg1;
  ///xg1[0] = pos/10000000;xg1[1] = (pos%10000000)/100000;xg1[2] = (pos%100000)/1000;xg1[3] = pos%1000;
  int t0 = pos%1000;

  EigenV res;ga_matrices_cps   ga_cps;
  meson_corrE(propa, propb, ga_cps.ga[a][b],ga_cps.ga[c][d],  res, fd);
  ///std::vector<double > write;write.resize(2*nt);
  for(int ti=0;ti<nt;ti++)
  {
    double v0 = res[ti].real();
    double v1 = res[ti].imag();
    write[offw + ((ti- t0 +nt)%nt)*2+0]= v0;
    write[offw + ((ti- t0 +nt)%nt)*2+1]= v1;
  }
  ////write_data(write,output);

}

void meson_corr_write(std::string prop_a, std::string prop_b, std::string src_n, std::string out_n, const Geometry &geo, int a=0, int b=0, int c=0 , int d=0)
{
  print_mem_info();
  io_vec io_use(geo, 16);
  fft_desc_basic fd(geo);
  EigenM propa,propb;
  qlat::vector_acc<int > nv, Nv, mv;
  geo_to_nv(geo, nv, Nv, mv);
  int nt = nv[3];

  qlat::FieldM<qlat::Complex,1> noi;
  noi.init(geo);
  //std::vector<Propagator4d > propVa;propVa.resize(0);propVa.resize(1);propVa[0].init(geo);
  //std::vector<Propagator4d > propVb;propVb.resize(0);propVb.resize(1);propVb[0].init(geo);
  Propagator4d propVa;propVa.init(geo);
  Propagator4d propVb;propVb.init(geo);


  char prop_na[500],prop_nb[500],noi_name[500];
  char output[500];
  sprintf(prop_na, "%s",prop_a.c_str() );
  sprintf(prop_nb, "%s",prop_b.c_str() );

  sprintf(noi_name ,"%s",src_n.c_str()  );
  sprintf(output,   "%s",out_n.c_str());

  print0("Noise %s \n",noi_name);
  print0("Prop  %s %s \n",prop_na, prop_nb);
  print0("output %s \n", output);

  qlat::set_zero(noi);
  load_gwu_noi(noi_name,noi ,io_use);
  load_gwu_prop(prop_na, propVa);
  if(prop_a == prop_b){propVb = propVa;}
  else{load_gwu_prop(prop_nb, propVb);}
  

  copy_propE(propVa,propa, fd );
  copy_propE(propVb,propb, fd );

  Coordinate pos;qlat::vector_acc<int > off_L;
  check_noise_pos(noi, pos, off_L);

  ////Coordinate xg1;
  ////xg1[0] = pos/10000000;xg1[1] = (pos%10000000)/100000;xg1[2] = (pos%100000)/1000;xg1[3] = pos%1000;

  EigenV res;ga_matrices_cps   ga_cps;
  meson_corrE(propa, propb, ga_cps.ga[a][b],ga_cps.ga[c][d],  res, fd);
  std::vector<double > write;write.resize(2*nt);
  for(unsigned int ti=0;ti<write.size()/2;ti++){
    double v0 = res[ti].real();
    double v1 = res[ti].imag();
    write[((ti-pos[3]+nt)%nt)*2+0]= v0;
    write[((ti-pos[3]+nt)%nt)*2+1]= v1;
  }

  write_data(write,output);

}

Coordinate get_src_pos(std::string src_n, qlat::vector_acc<int > &off_L, const Geometry &geo)
{
  io_vec io_use(geo, 16);
  char noi_name[500];
  sprintf(noi_name ,"%s",src_n.c_str()  );

  qlat::FieldM<qlat::Complex,1> noi;
  noi.init(geo);

  print0("Noise %s \n",noi_name);
  qlat::set_zero(noi);
  load_gwu_noi(noi_name,noi ,io_use);
  Coordinate pos;////qlat::vector<int > off_L;
  check_noise_pos(noi, pos,off_L);

  return pos;
}

template<typename Ty>
void print_pion(qlat::FieldM<Ty, 12*12 > propM)
{
  Geometry geo = propM.geo();
  fft_desc_basic fd(geo);

  Propagator4dT<Ty > prop4d;prop4d.init(geo);
  EigenM propE;

  copy_noise_to_prop(propM, prop4d, 1);
  copy_propE(prop4d, propE, fd);

  ga_matrices_cps   ga_cps;
  EigenV res;
  meson_vectorE(propE, propE, ga_cps.ga[0][0], ga_cps.ga[0][0],res, fd);

  int nv = res.size()/fd.nt;
  for(int iv=0;iv<nv;iv++)
  for(int t=0;t<res.size();t++)
  {
    Ty v = res[iv*fd.nt + t];
    print0("iv %d, t %d, v %.6e %.6e \n", iv, t, v.real(), v.imag());
  }



}

}


#endif

