#!/usr/bin/env bash
dest=/home/benji/hbs_root/
sudo rsync -av --delete server:/mdbase $dest
echo Press ENTER to sync to vm
read
sudo rsync -av --delete $dest/mdbase vm:/
