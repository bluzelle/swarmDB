#!/bin/bash -e
# A script for installing boost

if [ "$1" == "-h" ]; then
  echo "Usage: `basename $0` requires that BOOST_VERSION (e.g. 1.67.0) and BOOST_INSTALL_DIR are set in the local env"
  exit 0
fi

if [ -z "${BOOST_VERSION}" ]; then
    echo "Need to set BOOST_VERSION"
    exit 1
fi

if [ -z "${BOOST_INSTALL_DIR}" ]; then
    echo "Need to set BOOST_INSTALL_DIR"
    exit 1
fi

BOOST_LIBS="test,chrono,coroutine,program_options,random,regex,system,thread,log,serialization"
BOOST_VERSION_UNDERSCORES="$(echo "${BOOST_VERSION}" | sed 's/\./_/g')"
BOOST_TARBALL="boost_${BOOST_VERSION_UNDERSCORES}.tar.gz"
BOOST_URL="http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_TARBALL}"
BOOST_PACKAGE_DIR=${BOOST_INSTALL_DIR}/${BOOST_VERSION_UNDERSCORES}

if [ ! -e ${BOOST_PACKAGE_DIR}/libs ]; then
    echo "Boost not found in the cache, get and extract it from ${BOOST_URL}"
    mkdir -p ${BOOST_PACKAGE_DIR}
    curl -L ${BOOST_URL} | tar -xz -C ${BOOST_PACKAGE_DIR} --strip-components=1
    cd ${BOOST_PACKAGE_DIR}
    ./bootstrap.sh --prefix=${BOOST_PACKAGE_DIR} --with-libraries=${BOOST_LIBS}
    ./b2 install -d0
else
    echo "Using cached Boost"
fi

echo "BOOST_ROOT=${BOOST_PACKAGE_DIR}"
export BOOST_ROOT=${BOOST_PACKAGE_DIR}