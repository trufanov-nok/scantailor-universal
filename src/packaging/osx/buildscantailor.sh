#!/bin/bash

OURDIR=`dirname $0`
OURDIR=`cd $OURDIR; pwd`
STSRC=`cd $OURDIR/../../..; pwd`
STHOME=`cd $OURDIR/../../../..; pwd`
echo -e "Building ScanTailor Universal - Base Directory: $STHOME\n\n"

# I’ve hardcoded path to SDK [truf]
PATH_TO_SDK="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk"
# I’ve hardcoded path to Qt [truf]
PREFIX_PATH=$HOME"/build/Qt/5.8/clang_64"


export BUILDDIR=$STHOME/scantailor-universal-deps-build
export STBUILDDIR=$STHOME/scantailor-universal-build
export CMAKE_PREFIX_PATH=$BUILDDIR
export CMAKE_INCLUDE_PATH=$BUILDDIR
export CMAKE_LIBRARY_PATH=$BUILDDIR
export CMAKE_INSTALL_PREFIX=$BUILDDIR
mkdir -p $BUILDDIR

export CC=gcc-7
export CPP=cpp-7
export CXX=g++-7
export LD=g++-7

export MAC_OS_X_VERSION_MIN_REQUIRED=10.4
export MACOSX_DEPLOYMENT_TARGET=10.9
export MYCFLAGS="-m64 -arch x86_64 -mmacosx-version-min=10.4" # I’ve hardcoded -m64 and arch to 64bit only [truf]
export MYLDFLAGS="-m64 -isysroot $PATH_TO_SDK" # and here too


function build_lib {
  libname=$1
  libdir=$2
 
  LIB_DYLIB=`find $BUILDDIR -iname $1`
  if [ "x$LIB_DYLIB" == "x" ]
  then
    echo "Library $1 not built, trying to build..."
    LIB_DIR=`find $STHOME -type d -maxdepth 1 -iname "$2"`
    if [ "x$LIB_DIR" == "x" ]
    then
      echo "Cannot find library source code.  Check in $STHOME for $2 directory."
      exit 0
    fi
    cd $LIB_DIR
    # make clean
    if [ ! -r Makefile ]
    then
       if [ -r configure ]
       then
            env CFLAGS="$MYCFLAGS" CXXFLAGS="$MYCFLAGS" LDFLAGS="$MYLDFLAGS" ./configure --prefix=$BUILDDIR --disable-dependency-tracking --enable-shared --enable-static
       fi

       if [ -r CMakeLists.txt ]
       then
            mkdir build
            cd build
            cmake .. -DCMAKE_PREFIX_PATH=“~/build/Qt/5.5/clang_64”
       fi
    fi
    make
    make install
    cd $STHOME
  fi
  return 0
}

# Check for and build dependency libraries if needed
build_lib "libtiff.dylib" "tiff-[0-9]*.[0-9]*.[0-9]*"
build_lib "libjpeg.dylib" "jpeg-[0-9]*"
build_lib "libpng.dylib" "libpng-[0-9]*.[0-9]*.[0-9]*"
build_lib "libopenjp2.dylib" "openjpeg-[0-9]*.[0-9]*.[0-9]*"

# BOOST
#curl -L -o  boost_1_43_0.tar.gz "http://sourceforge.net/projects/boost/files/boost/1.43.0/boost_1_43_0.tar.gz/download"
#tar xzvvf boost_1_43_0.tar.gz
export BOOST_DIR=`find $STHOME -type d -iname boost_[0-9]*_[0-9]*_[0-9]*`
if [ "x$BOOST_DIR" == "x" ]
then
   echo "Cannot find BOOST libraries.  Check in $STHOME for boost_x_x_x directory."
   exit 0lip
fi
BOOST_LIB3=`find $BUILDDIR/lib -iname libboost_prg_exec_monitor.a`
BOOST_LIB5=`find $BUILDDIR/lib -iname libboost_unit_test_framework.a`
if [ "x$BOOST_LIB3" == "x" ] || \
   [ "x$BOOST_LIB5" == "x" ]
then
  cd $BOOST_DIR
  [ ! -x ./bjam ] && ./bootstrap.sh --prefix=$BUILDDIR --with-libraries=test,system
  echo ./bjam --toolset=darwin-4.0 --prefix=$BUILDDIR --user-config=$OURDIR/user-config.jam --build-dir=$BUILDDIR --with-test --with-system link=static runtime-link=static architecture=combined address-model=64 macosx-version=$MACOSX_DEPLOYMENT_TARGET macosx-version-min=$MAC_OS_X_VERSION_MIN_REQUIRED --debug-configuration install
  ./bjam --toolset=darwin-4.0 --prefix=$BUILDDIR --user-config=$OURDIR/user-config.jam --build-dir=$BUILDDIR --with-test --with-system link=static runtime-link=static architecture=combined address-model=64 macosx-version=$MACOSX_DEPLOYMENT_TARGET macosx-version-min=$MAC_OS_X_VERSION_MIN_REQUIRED --debug-configuration install
fi
export BOOST_ROOT=$BUILDDIR
cd $STHOME

# SCANTAILOR
cd $STSRC
# make clean
# rm CMakeCache.txt
# needed in case scantailor source is not updated to compile with new boost (>=1_34) test infrastructure
#[ ! -f $STSRC/src/imageproc/tests/main.cpp.old ] && sed -i '.old' -e '1,$ s%^#include <boost/test/auto_unit_test\.hpp>%#include <boost/test/included/unit_test.hpp>%g' $STSRC/src/imageproc/tests/main.cpp # hardcoded in cpp now [truf]
#[ ! -f $STSRC/tests/main.cpp.old ] && sed -i '.old' -e '1,$ s%^#include <boost/test/auto_unit_test\.hpp>%#include <boost/test/included/unit_test.hpp>%g' $STSRC/tests/main.cpp #hardcoded in cpp now [truf]
[ ! -f CMakeCache.txt ] && cmake -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET -DCMAKE_OSX_SYSROOT="$PATH_TO_SDK" -DPNG_INCLUDE_DIR=$BUILDDIR -DCMAKE_PREFIX_PATH=$PREFIX_PATH .
# I’ve hardcoded path to Qt above [Truf]
make
echo "$OURDIR/makeapp.sh $STBUILDDIR $STSRC $BUILDDIR"
$OURDIR/makeapp.sh $STBUILDDIR $STSRC $BUILDDIR
cd $STHOME

exit 0
