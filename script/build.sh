export PATH=/opt/cross-tools/bin:$PATHcross-tools-gcc-full/cross-tools/bin:$PATH

mkdir -p ../loongarch-build
sudo mkdir -p /opt/grub-loongarch

cd ../loongarch-build
../configure --target=loongarch64-unknown-linux-gnu --with-platform=efi --prefix=/opt/grub-loongarch --disable-grub-mkfont --disable-werror
make -j$(nproc)
sudo env PATH=$PATH make install