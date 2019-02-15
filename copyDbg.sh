#!/bin/sh
echo
echo $1 # source directory to copy from
echo $2 # target directory to copy to
echo $3 # ip of target machine

user=root
pw=keba
ip=$3


sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "chmod +x /usr/bin/gdb"
sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "rm -r $2"
sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "mkdir -v -p $2"
#sshpass -p "$pw" ssh $user@$ip "rm -rf $1"
#sshpass -p "$pw" ssh $user@$ip "mkdir -v -p $1"

sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "rm $2/*"
sshpass -p "$pw" scp -r -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $1/bin/* $user@$ip:$2
#sshpass -p "$pw" scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $1/../bin/* $user@$ip:$2
sshpass -p "$pw" scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $1/lib/* $user@$ip:$2
#sshpass -p "$pw" scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $1/../lib/* $user@$ip:$2

echo "create library symlinks without adding the directory to the global library paths"
#sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "LD_LIBRARY_PATH=$2"
#sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "EXPORT LD_LIBRARY_PATH"
sshpass -p "$pw" ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $user@$ip "ldconfig -N '$2'"



echo "Done"
