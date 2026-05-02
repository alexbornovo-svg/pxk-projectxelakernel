#include "cpu.h"
#include "../drivers/ata.h"
#include "../lib/kstd.h"
#include "../lib/string.h"
#include "../lib/printf.h"

extern uint32_t multiboot_mem_upper;
extern uint32_t multiboot_mem_lower;

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf)
    );
}

void cpu_get_info(cpu_info_t *out)
{
    uint32_t eax, ebx, ecx, edx;

    cpuid(0x00000000, &eax, &ebx, &ecx, &edx);
    ((uint32_t *)out->vendor)[0] = ebx;
    ((uint32_t *)out->vendor)[1] = edx;
    ((uint32_t *)out->vendor)[2] = ecx;
    out->vendor[12] = '\0';

    cpuid(0x00000001, &eax, &ebx, &ecx, &edx);
    out->stepping = eax & 0xF;
    out->model    = (eax >> 4)  & 0xF;
    out->family   = (eax >> 8)  & 0xF;

    uint32_t max_ext;
    cpuid(0x80000000, &max_ext, &ebx, &ecx, &edx);

    if (max_ext >= 0x80000004)
    {
        uint32_t *brand = (uint32_t *)out->brand;
        cpuid(0x80000002, &brand[0],  &brand[1],  &brand[2],  &brand[3]);
        cpuid(0x80000003, &brand[4],  &brand[5],  &brand[6],  &brand[7]);
        cpuid(0x80000004, &brand[8],  &brand[9],  &brand[10], &brand[11]);
        out->brand[48] = '\0';
    }
    else
    {
        strncpy(out->brand, "unknown", 48);
    }
}

void cpu_get_sys_info(sys_info_t *out)
{
    out->total_mb = (multiboot_mem_upper + multiboot_mem_lower) / 1024;

    uint16_t identify[256];
    extern int ata_identify(uint16_t *buf);
    if (ata_identify(identify) == 0)
    {
        uint32_t sectors =
            ((uint32_t)identify[61] << 16) | (uint32_t)identify[60];
        out->disk_mb = (sectors / 2) / 1024;
    }
    else
    {
        out->disk_mb = 0;
    }
}

static uint print_uint(uint32_t n, uint line, uchar color)
{
    char buf[12];
    int  i = 10;
    buf[11] = '\0';
    if (n == 0)
    {
        buf[i--] = '0';
    }
    else
    {
        while (n > 0)
        {
            buf[i--] = '0' + (n % 10);
            n /= 10;
        }
    }
    return vga_write(buf + i + 1, line, color);
}

void cpu_print_info(uint line)
{
    cpu_info_t  cpu;
    sys_info_t  sys;
    cpu_get_info(&cpu);
    cpu_get_sys_info(&sys);

    char tmp[64];

    line = printf(WHITE, line, "CPU vendor: %s \n", cpu.vendor);

    line = printf(WHITE, line, "CPU model: %s \n\n", cpu.brand);

    line = printf(WHITE, line, "RAM: %u \n", sys.total_mb);

    line = printf(WHITE, line, "DISK: %u \n", sys.disk_mb);
}