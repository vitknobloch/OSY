#!/bin/sh

# Clone repos
git clone git://git.busybox.net/busybox
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git

# Compile busybox
(cd ./busybox && make defconfig)
(cd ./busybox && make -j$(nproc))

# Compile linux kernel
(cd ./linux && make defconfig)
(cd ./linux && make -j$(nproc))

# Create filesystem
cd ./busybox
make install

# Copy libraries
mkdir _install/lib
mkdir _install/lib/x86_64-linux-gnu
mkdir _install/lib64
cp /lib/x86_64-linux-gnu/libm.so.6 _install/lib/x86_64-linux-gnu/
cp /lib/x86_64-linux-gnu/libresolv.so.2 _install/lib/x86_64-linux-gnu/
cp /lib/x86_64-linux-gnu/libc.so.6 _install/lib/x86_64-linux-gnu/
cp /lib64/ld-linux-x86-64.so.2 _install/lib64

# Create bootable image
mkdir _install/dev _install/etc _install/proc _install/sys
mkdir _install/etc/init.d
printf "#!/bin/sh\nmount -t proc none /proc\necho Busybox ready.\n" >_install/etc/init.d/rcS
chmod +x _install/etc/init.d/rcS
sudo cp -a /dev/tty? _install/dev
ln -s bin/busybox _install/init
(cd _install; find . | cpio -o -H newc | gzip) > ramdisk

# Boot qemu
cd ..
qemu-system-x86_64 -kernel linux/arch/x86/boot/bzImage -initrd busybox/ramdisk
