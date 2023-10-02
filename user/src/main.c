// initial entry point, called by kernel

// user
#if defined(K1)
    #include "k1.h"
#elif defined(K2)
    #include "k2.h"
#endif

void user_main(void) {
    // run whichever kernel test program, or Train Control (later)
    #if defined(K1)
        kernel1_test();
    #elif defined(K2)
        kernel2_test();
    #endif
}
