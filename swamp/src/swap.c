#include "swap.h"
#include "networking.h"


int main(){

    swapConfig = getswapConfig("./cfg/swap.config");

    int serverSocket = createListenServer(swapConfig->swapIP,swapConfig->swapPort);

    int newProcessSocket = getNewClient(serverSocket);

    while(1){
        
    }
        

    return 0;
}