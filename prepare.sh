#!/bin/bash

git submodule init
git submodule update

cd YCSB-C
git apply ../patches/YCSB-C.patch >/dev/null 2>&1 || git apply ../patches/YCSB-C.patch -R --check
cd ..

wget https://www.sqlite.org/2017/sqlite-amalgamation-3190300.zip
unzip -o sqlite-amalgamation-3190300.zip
cp sqlite-amalgamation-3190300/sqlite3.c sqlite-amalgamation-3190300/sqlite3.h YCSB-C/sqlite/

cp YCSB-C/sqlite/sqlite3.c YCSB-C/sqlite/sqlite3_sgx.c
patch YCSB-C/sqlite/sqlite3_sgx.c < patches/sqlite3_sgx.patch

rm -rf sqlite-amalgamation-3190300 sqlite-amalgamation-3190300.zip
