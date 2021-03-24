#!/usr/bin/env bash
host=server
file=INVENT
echo Make sure /nas is read-write
echo Press ENTER to copy the first file
read
sudo scp $host:/mdbase/loc1/$file /nas/Documents/Benji/compare/A
echo Press ENTER to copy the next file
read
sudo scp $host:/mdbase/loc1/$file /nas/Documents/Benji/compare/B
echo Done
