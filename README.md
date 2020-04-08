# Weenix-OS

This file contains quick instructions for getting Weenix to run on
Redhat-derived or Debian-derived Linux flavors.  See the documentation in doc/
for detailed instructions.

1. Download and install dependencies.

   On recent versions of Ubuntu or Debian, you can simply run:

   $ sudo apt-get install git-core build-essential gcc gdb qemu genisoimage make python python-argparse cscope xterm bash grub xorriso

   or on Redhat:

   $ sudo yum install git-core gcc gdb qemu genisoimage make python python-argparse cscope xterm bash grub2-tools xorriso

2. Compile Weenix:

   $ make

3. Invoke Weenix:

   $ ./weenix -n

   or, to run Weenix under gdb, run:

   $ ./weenix -n -d gdb
