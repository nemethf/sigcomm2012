Towards SmartFlow: Case Studies on Enhanced Programmable Forwarding in OpenFlow Switches
========================================================================================

Sigcomm2012 demo


INSTALLATION
------------

* You can download a prebuilt VirtualBox image from [our project page](http://sb.tmit.bme.hu/mediawiki/index.php/Sigcomm2012).
* Alternatively, you can create your own VM image by following these steps:
    1. Download the [official mininet image](http://yuba.stanford.edu/foswiki/bin/view/OpenFlow/MininetGettingStarted).  (We used [vm-ubuntu11.10-052312.vmware](https://github.com/downloads/mininet/mininet/mininet-vm-ubuntu11.10-052312.vmware.zip)).
    2. Create a new virtual machine in VirtualBox with the downloaded disk image.
    3. Start the VM.  Login with openflow/openflow.  
    4. Mount the Guest Additions disk image by selecting Devices>Install Guest Additions menu item in the VirtualBox menu bar.
    5. Inside the VM, type:
	
	```
    wget https://github.com/nemethf/sigcomm2012/raw/master/install_demo.sh
    sh ./install_demo.sh
	```

RUNNING THE DEMO
----------------

Log in, type: `startx`, and follow the [walkthrough text](https://github.com/nemethf/sigcomm2012/blob/master/butterfly_app/walkthrough.org) appearing in the top-right corner.

CONTACT
-------

[nemethf@tmit.bme.hu](mailto:nemethf@tmit.bme.hu)


