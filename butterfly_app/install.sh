#!/bin/sh

# This file installs the butterfly application into NOX.  Call with
# two parameters.  The first parameter is the top level direcory of
# NOX, the second one is the top level directory of the OpenFlow 1.1
# softswitch.

NOX_DIR=$1
SS_DIR=$2

FILE=$NOX_DIR/configure.ac.in
sed -i -e 's/\(#add.coreapps.component.here\)/butterfly_app\n\t\t\1/' $FILE

FILE=$NOX_DIR/src/builtin/Makefile.am
sed -i -e 's/\(..oflib\/liboflib.la\)/\1 ..\/oflib-exp\/liboflib_exp.la/' $FILE

FILE=$NOX_DIR/src/oflib-exp/Makefile.am
NEW_FILES=' \\\n\tofl-exp-bme.c \\\n\tofl-exp-bme.h'
sed -i -e 's/\(ofl-exp-openflow.h\)/\1'"$NEW_FILES"/ $FILE

cp $SS_DIR/oflib/* $NOX_DIR/src/oflib
cp $SS_DIR/oflib-exp/* $NOX_DIR/src/oflib-exp

cp $SS_DIR/include/openflow/* $NOX_DIR/src/include/openflow/openflow

#
echo "now you should: cd $NOX_DIR; ./boot.sh; ./configure; make"
