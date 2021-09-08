#include <cspecs/cspec.h>
#include <stdio.h>
#include "prueba1.h"


context(pruebAAAA){

    describe("PROBANDO"){

        before{
            printf("Antsasdasd324823794es\n");
        }end
        after{
            printf("Dessasasdasdsspues\n");
        }end

        it("Deberia devolver 3."){
            should_int(funcionPrueba()) be equal to(3);
        }end
    }end
}

