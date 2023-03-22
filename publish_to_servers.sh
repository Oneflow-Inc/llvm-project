#!/usr/bin/env bash
# Usage: ./publish_to_servers.sh clangd-14.AppImage

set -euxo pipefail

function upload {
  scp $1 $2:$3
  # scp $1 $3:/tmp
  # bn=$(basename $1)
  # # `sudo` is intercepted by /opt/rh/devtoolset-7/root/usr/bin/sudo in some machines
  # ssh $3 "echo $2 | /usr/bin/sudo -S cp /tmp/$bn /usr/local/bin"
  # ssh $3 "rm /tmp/$bn"
}

for x in `seq 11 16`
do
  upload $1 $x-suzhu /dataset/
done

for x in `seq 21 23`
do
  upload $1 $x-suzhu /dataset/
done

for x in `seq 25 26`
do
  upload $1 $x-suzhu /data/dataset
done
