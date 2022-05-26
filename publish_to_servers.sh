#!/usr/bin/env bash
# Usage: ./publish_to_servers.sh clangd-14.AppImage sudo_passwd

set -euxo pipefail

function upload {
  scp $1 $3:/tmp
  # `sudo` is intercepted by /opt/rh/devtoolset-7/root/usr/bin/sudo in some machines
  ssh $3 "echo $2 | /usr/bin/sudo -S cp /tmp/$1 /usr/local/bin"
  ssh $3 "rm /tmp/$1"
}

for x in `seq 11 16`
do
  upload $1 $2 oneflow-$x
done

for x in `seq 21 23`
do
  upload $1 $2 $x-suzhu
done
