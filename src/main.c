#include <stdlib.h>

#include "app.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef _WIN32
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    app_init();
    app_run();
    app_fini();

    return EXIT_SUCCESS;
}
