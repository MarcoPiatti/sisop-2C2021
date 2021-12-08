#!/bin/bash
length=$(($#-1))
OPTIONS=${@:1:$length}
REPONAME="${!#}"
CWD=$PWD
echo -e "\n\nInstalling commons libraries...\n\n"
COMMONS="so-commons-library"
git clone "https://github.com/sisoputnfrba/${COMMONS}.git" $COMMONS
cd $COMMONS
sudo make uninstall
make all
sudo make install
cd $CWD
echo -e "\n\nBuilding...\n\n"
make -C ./kernel
make -C ./memory
make -C ./swamp
make -C ./libmate
sudo cp ./libmate/libmatelib.so /usr/lib
echo -e "\n\nDeploy terminado!\n\n"
