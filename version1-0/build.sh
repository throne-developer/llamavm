rm -rf ./depcomp ./missing
rm -rf ./configure
autoheader
aclocal
autoconf
automake -a
./configure
make

rm -rf ./depcomp ./missing
rm -rf Makefile.in configure config.log config.status autom4te.cache/ aclocal.m4 install-sh *.o
