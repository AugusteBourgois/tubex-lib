#!/bin/bash

set -x

cd $HOME
echo 'Installing IBEX in ' $HOME '...';
if [ ! -e "ibex-lib/README.md" ]; then
  git clone -b develop https://github.com/ibex-team/ibex-lib.git ;
  cd ibex-lib ;
  #git checkout 47fc75a5e14eb2f353ae7e0b37ac0f9dad7801c0 ;
  mkdir build && cd build ;
  cmake -DCMAKE_CXX_FLAGS="-fPIC -Wno-cpp -Wno" -DCMAKE_C_FLAGS="-fPIC -Wno-cpp -Wno" -DINTERVAL_LIB=filib -DCMAKE_INSTALL_PREFIX=$HOME/ibex-lib/build_install .. ;
  make ;
else
  echo 'Using cached directory.' ;
fi
cd $HOME/ibex-lib/build
make install
cd $HOME