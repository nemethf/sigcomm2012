#!/bin/sh

#
# dependencies
#
sudo apt-get update
sudo apt-get -y install \
 vlc wmaker pyflakes xserver-xorg-video-vmware xinit \
 libgoo-canvas-perl libcanberra-gtk-module
sudo apt-get clean

#
# VirtualBox Guest Additions
#
sudo mount /dev/cdrom2 /mnt
cd /mnt
if [ -f ./VBoxLinuxAdditions.run ]; then
    sudo ./VBoxLinuxAdditions.run
else
    sudo apt-get -y install virtualbox-guest-additions
fi

#
# OF softswitch with BME extensions
#
SS=of11softswitch.bme
SS_DIR=~/$SS
cd
git clone -b sigcomm2012 git://github.com/nemethf/of11softswitch.git $SS
cd $SS_DIR
./boot.sh
./configure
make
sudo make install
make clean

#
# nox & demo application
#
cd
NOX=nox11oflib
APP=sigcomm2012/butterfly_app
NOX_DIR=~/$NOX
APP_DIR=$NOX_DIR/src/nox/coreapps/butterfly_app
git clone git://github.com/TrafficLab/nox11oflib.git
git clone -b master git://github.com/nemethf/sigcomm2012.git
# linking doesn't work, so we must copy:
cp -a ~/$APP $APP_DIR
cd $APP_DIR
./install.sh $NOX_DIR $SS_DIR
cd $NOX_DIR
./boot.sh
./configure
make
sudo make install

#
# mininet
#
MININET=mininet.bme
cd
git clone -b master git://github.com/nemethf/mininet.git $MININET
cd $MININET
sudo make install

#
# video files
#

#
# misc
#
cat <<EOF | sudo tee -a /etc/issue


sigcomm2012 demo
================
username: openflow
password: openflow

EOF
cat >> ~/.bashrc <<EOF
export NOX_CORE_DIR=$NOX_DIR/src
EOF
export NOX_CORE_DIR=$NOX_DIR/src
WM_DIR=~/GNUstep/Library/WindowMaker
mkdir -p $WM_DIR
cat >> $WM_DIR/autostart <<EOF
xterm -geometry +0+0 -T Mininet -e sudo -E mn --custom $APP_DIR/butterfly.py &
emacs --no-splash -f tool-bar-mode -g 70x23-0+0 $APP_DIR/walkthrough.org &
EOF
chmod u+x $WM_DIR/autostart
INFO="You can start the demo by typing: startx"
echo echo $INFO >> ~/.profile
