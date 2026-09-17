#include <stdlib.h>

#include "log.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    B_LOG(B_DEBUG, "foo, bar");
    B_LOG(B_INFO, "foo, bar");
    B_LOG(B_WARN, "foo, bar");
    B_LOG(B_ERROR, "foo, bar");
    B_LOG(B_FATAL, "foo, bar");
    return EXIT_SUCCESS;
}
