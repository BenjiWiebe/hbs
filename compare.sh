#!/usr/bin/env bash
# shellcheck disable=SC2162
file=TRANSACT.IDX
file2=TRANSACT
srchost=vm
srcdir=/mdbase/loc1
destdir=/nas/Documents/Benji/compare
echo Make sure /nas is read-write
echo Press ENTER to copy the first file
read
sudo scp $srchost:$srcdir/$file $destdir/A1
test x$file2 = x || sudo scp $srchost:$srcdir/$file2 $destdir/A2
echo Press ENTER to copy the next file
read
sudo scp $srchost:$srcdir/$file $destdir/B1
test x$file2 = x || sudo scp $srchost:$srcdir/$file2 $destdir/B2
echo Done
