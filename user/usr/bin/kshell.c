/*
 * Runs the in-kernel shell.
 */

#include "wenix/syscall.h"
#include "wenix/trap.h"

int main(int argc, char **argv)
{
        return trap(SYS_kshell, (uint32_t)0);
}
