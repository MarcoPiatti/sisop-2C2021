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
cd $CWD
echo -e "\n\n Instalando los carpinchos de prueba... \n\n"
git clone "https://github.com/sisoputnfrba/carpinchos-pruebas.git"
cd carpinchos-pruebas
make compile 
echo -e "\n\nCarpinchos de prueba compilados!\n\n"
echo -e "\n\nCreando carpetas de dumps...\n\n"
mkdir ~/dumps
mkdir ~/dumps/tlb
echo -e "\n\nDeploy terminado!\n\n"
