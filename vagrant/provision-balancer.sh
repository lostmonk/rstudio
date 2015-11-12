#!/usr/bin/env bash

# amend MOTD
echo "Load balanced: /etc/rstudio/load-balancer"

# configure primary machine for load balancing
ssh -i /rstudio/vagrant/vagrant_key_rsa -o StrictHostKeyChecking=no vagrant@192.168.55.101 "sudo /rstudio/vagrant/provision-primary-load.sh" 

# set up cluster node config file
cp /rstudio/vagrant/load-balancer /etc/rstudio/load-balancer

# create secure cookie key
apt-get install -y uuid
sh -c "echo `uuid` > /etc/rstudio/secure-cookie-key"
chmod 0600 /etc/rstudio/secure-cookie-key

# copy key to primary machine
scp -i /rstudio/vagrant/vagrant_key_rsa -o StrictHostKeyChecking=no /etc/rstudio/secure-cookie-key vagrant@192.168.55.101:/etc/rstudio

# connect to NFS server already running on the primary machine
apt-get install nfs-common
mkdir -p /primary/home 
echo "192.168.55.101:/home /primary/home nfs rsize=8192,wsize=8192,timeo=14,intr" >> /etc/fstab
mount -a 

# add users; use NFS home directories
apt-get install -y whois
for userdetails in `cat /rstudio/vagrant/rstudiousers.txt`
do
    user=`echo $userdetails | cut -f 1 -d ,`
    passwd=`echo $userdetails | cut -f 2 -d ,`
    useradd --base-dir /primary/home -p `mkpasswd $passwd` $user
done

# configure shared storage
echo "server-shared-storage-path=/primary/home/shared-storage" >> /etc/rstudio/rserver.conf

# TODO Should we route /vagrant's home dir via NFS, too? Needed if we want to 
# be able to have load-balanced sessions as the vagrant user.

# perform remainder of the install script as regular user
sudo --login --set-home -u vagrant /rstudio/vagrant/provision-balancer-user.sh
