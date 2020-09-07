#/bin/sh

#listing dir called test
echo 'Before inserting module.'
ls -a test
echo

make 1>/dev/null

sudo insmod hide_fileko.ko
echo 'After inserting module'

ls -a test
echo
sudo rmmod hide_fileko.ko

echo 'After romoving module.' 
ls -a test

make clean 1>/dev/null
