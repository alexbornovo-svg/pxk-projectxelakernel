#ifndef SHELL_KERNEL_H
#define SHELL_KERNEL_H

#include "types.h"
#include "../lib/xelagraph.h"

void kernel_shell_run(uint line, multiboot_info_t *mbi);
#endif