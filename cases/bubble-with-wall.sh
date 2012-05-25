#! /bin/bash

set -e
set -u

cfgname=bubble-with-wall
dname=${cfgname}1
mkdir -p ${dname}
sph=../../src/sph
cd ${dname}

cp ../${cfgname}.cfg ${cfgname}.cfg
awk --lint=fatal '/INITIAL_CONDITION/{$2=0} 1' ../${cfgname}.cfg | \
    awk '/TIMING/{$3=1e-6; $4=1e-6} 1' > ${cfgname}_init.cfg
${sph}  ${cfgname}_init
awk --lint=fatal -f ../../scripts/${cfgname}.awk  ${cfgname}_init.rst > ${cfgname}.rst

rm outdata/*.dat
${sph}  ${cfgname}