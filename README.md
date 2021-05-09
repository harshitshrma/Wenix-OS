# Wenix-OS

This file contains quick instructions for getting Wenix to run on
Redhat-derived or Debian-derived Linux flavors.  See the documentation in doc/
for detailed instructions.

1. Download and install dependencies.

   On recent versions of Ubuntu or Debian, you can simply run:

   $ sudo apt-get install git-core build-essential gcc gdb qemu genisoimage make python python-argparse cscope xterm bash grub xorriso

   or on Redhat:

   $ sudo yum install git-core gcc gdb qemu genisoimage make python python-argparse cscope xterm bash grub2-tools xorriso

2. Compile Wenix:

   $ make

3. Invoke Wenix:

   $ ./wenix -n

   or, to run Wenix under gdb, run:

   $ ./wenix -n -d gdb
