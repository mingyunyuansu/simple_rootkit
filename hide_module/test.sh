# /bin/sh

cd ../hide_process/
make >/dev/null 2>&1
sudo insmod hide_processko.ko
cd ../hide_module/

make >/dev/null 2>&1
echo 'Before inserting module'
lsmod | grep hide
echo

sudo insmod hide_moduleko.ko
echo 'After inserting module'
lsmod | grep hide
echo

sudo rmmod hide_moduleko.ko
echo 'After removing module' 
lsmod | grep hide

sudo rmmod hide_processko.ko
make clean >/dev/null 2>&1
cd ../hide_process/
make clean >/dev/null 2>&1

