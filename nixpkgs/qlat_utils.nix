{ fetchPypi
, stdenv
, lib
, buildPythonPackage
, cython
, meson-python
, pkg-config
, numpy
, psutil
, zlib
, eigen
, git
, is-pypi-src ? true
, openmp ? null
}:

buildPythonPackage rec {

  pname = "qlat_utils";
  version = if is-pypi-src then version-pypi else version-local;

  pyproject = true;

  src = if is-pypi-src then src-pypi else src-local;

  version-pypi = "0.70";
  src-pypi = fetchPypi {
    inherit pname;
    version = version-pypi;
    extension = "tar.gz";
    hash = "sha256-cfvie6jiE1VWyNPjKgQK26P6hpCgYr008WIIIpEipPA=";
  };

  version-local = "${../VERSION}-current";
  src-local = ../qlat-utils;

  enableParallelBuilding = true;

  inherit stdenv;

  build-system = [
    meson-python
    pkg-config
    cython
    numpy
  ];

  nativeBuildInputs = [
    git
  ];

  propagatedBuildInputs = [
    zlib
    eigen
  ]
  ++ lib.optional stdenv.cc.isClang openmp;

  dependencies = [
    meson-python
    pkg-config
    cython
    numpy
    psutil
  ];

  postPatch = ''
    sed -i "s/'-j4'/'-j$NIX_BUILD_CORES'/" pyproject.toml
  '';

}
