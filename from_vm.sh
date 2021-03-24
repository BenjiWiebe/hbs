#!/usr/bin/env bash
dest=/home/benji/hbs_root/
echo Syncing from vm to $dest/mdbase
sudo rsync -av --delete vm:/mdbase $dest
