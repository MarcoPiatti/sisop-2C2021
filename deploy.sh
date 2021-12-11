#!/bin/bash
length=$(($#-1))
OPTIONS=${@:1:$length}
REPONAME="${!#}"

if [[ $# -ne 3 ]]; then
    echo "Ingrese las tres IPs en orden Kernel -> Memoria -> Swap" >&2
    exit 2
fi

IPKERNEL="$1"
IPMEMORIA="$2"
IPSWAP="$3"

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
cd $CWD

echo -e "\n\nCambiando IPs configs...\n\n"
for i in ./kernel/cfg/*.config; do
    sed -i "s/IP_KERNEL=127.0.0.1/IP_KERNEL=$IPKERNEL/g" $i
    sed -i "s/IP_MEMORIA=127.0.0.1/IP_KERNEL=$IPMEMORIA/g" $i
done
for i in ./memory/cfg/*.config; do
    sed -i "s/IP=127.0.0.1/IP=$IPMEMORIA/g" $i
    sed -i "s/IP_SWAP=127.0.0.1/IP_SWAP=$IPSWAP/g" $i
done
for i in ./swamp/cfg/*.config; do
    sed -i "s/IP=127.0.0.1/IP=$IPSWAP/g" $i
done
sed -i "s/IP_MATE=127.0.0.1/IP_MATE=$IPKERNEL/g" ./configs-carpinchos/aKernel.config
sed -i "s/IP_MATE=127.0.0.1/IP_MATE=$IPMEMORIA/g" ./configs-carpinchos/aMemoria.config
echo -e "IPs cambiadas!\n\n"

echo -e "\n\nCopiando configs carpinchos...\n\n"
cd $CWD
cp ./configs-carpinchos/aKernel.config ./carpinchos-pruebas/build/aKernel.config
cp ./configs-carpinchos/aMemoria.config ./carpinchos-pruebas/build/aMemoria.config
echo -e "\nConfig carpinchos copiadas!\n\n"

echo -e "\n\nDeploy terminado!\n\n"
