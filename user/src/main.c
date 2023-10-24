// initial entry point, called by kernel

// user
#if defined(K1)
    #include "k1.h"
#elif defined(K2)
    #include "k2.h"
#elif defined(K3)
    #include "k3.h"
#elif defined(K4)
    extern void manual_track_controller(void);
#else
    #include "rpi.h"
#endif

void user_main(void) {
    // run whichever kernel test program, or Train Control (later)
    #if defined(K1)
        kernel1_test();
    #elif defined(K2)
        kernel2_test();
    #elif defined(K3)
        kernel3_test();
    #elif defined(K4)
        manual_track_controller();
    #else
        uart_puts(CONSOLE, "No Kernel was loaded, please compile with make k1/k2/k3/k4\r\n");
    #endif
}
