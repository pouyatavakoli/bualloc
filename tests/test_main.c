#include "test_halloc.h"
#include "test_hfree.h"
#include "test_hinit.h"

int main(){
    test_hinit();
    test_halloc();
    test_hfree();
    
    return 0;
}