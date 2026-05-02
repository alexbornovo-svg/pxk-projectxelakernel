#ifndef CPU_H
#define CPU_H

#include "types.h"

typedef struct
{
    char     vendor[13];
    char     brand[49];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
} cpu_info_t;

typedef struct
{
    uint32_t total_mb;
    uint32_t disk_mb;
} sys_info_t;

void cpu_get_info(cpu_info_t *out);
void cpu_get_sys_info(sys_info_t *out);
void cpu_print_info(uint line);

#endif