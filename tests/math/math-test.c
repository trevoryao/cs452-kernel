#include "math.h"
#include "uassert.h"

void user_main(void) {
    uassert(sqrt(16) == 4);
    uassert(sqrt(5929) == 77);
    uassert(sqrt(920512) == 959);
    uassert(sqrt(9367418611) == 96785);
}
