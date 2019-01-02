// vim: set ts=2 sw=2 expandtab:

#pragma once

#include <complex>
#include <cassert>
#include <unistd.h>

#ifdef OLD_CPP
#include <array-compatible.h>
#else
#include <array>
#endif

#define QLAT_START_NAMESPACE namespace qlat {
#define QLAT_END_NAMESPACE }

#ifndef USE_SINGLE_NODE
#define USE_MULTI_NODE
#endif

#define USE_NAMESPACE

#include <qutils/qutils.h>
#include <qutils/show.h>
#include <qutils/timer.h>
#include <qutils/rng-state.h>
#include <qutils/lat-io.h>

// #define SKIP_ASSERT

#ifdef SKIP_ASSERT
#define qassert(x) assert(true)
#else
#define qassert(x) { \
  if (not (x)) { \
    displayln("qassert failed: " #x); \
    usleep((useconds_t)(10.0 * 1.0e6)); \
    assert(false); \
  } \
}
#endif

QLAT_START_NAMESPACE

using namespace qutils;
using namespace qshow;
using namespace qtimer;
using namespace qrngstate;
using namespace latio;

const int DIMN = 4;

const int NUM_COLOR = 3;

typedef std::complex<double> Complex;

typedef std::complex<float> ComplexF;

inline const std::string& cname()
{
  static const std::string s = "Qlat";
  return s;
}

inline void warn(const std::string& str = "")
{
  if (str != "") {
    displayln_info(ssprintf("WARNING: %s", str.c_str()));
  }
}

QLAT_END_NAMESPACE

#include <qlat/coordinate.h>
