// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/driver_app.h"

// Run the cspcc executable.
int main(int argc, char **argv)
{
    cspcc::DriverApp app(argc, argv);
    return app.run();
}
