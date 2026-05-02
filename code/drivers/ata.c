#include "ata.h"
#include "io.h"
#include "types.h"

static uint16_t ata_base  = 0;
static uint8_t  ata_drive = 0;
static int      ata_ready = 0;

static const uint16_t ata_bases[2]  = { ATA_PRIMARY_BASE, ATA_SECONDARY_BASE };
static const uint8_t  ata_drives[2] = { ATA_MASTER, ATA_SLAVE };

static void ata_400ns_delay(uint16_t base)
{
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
}

static int ata_wait_bsy(uint16_t base)
{
    int timeout = 100000;
    while ((inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY) && timeout--)
        ;
    return timeout <= 0 ? -1 : 0;
}

static int ata_wait_drq(uint16_t base)
{
    int timeout = 100000;
    uint8_t status;
    while (timeout--)
    {
        status = inb(base + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR)
            return -1;
        if (status & ATA_STATUS_DRQ)
            return 0;
    }
    return -1;
}

static int ata_probe(uint16_t base, uint8_t drive)
{
    outb(base + ATA_REG_DRIVE, drive);
    ata_400ns_delay(base);

    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0xFF || status == 0x00)
        return -1;

    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_400ns_delay(base);

    status = inb(base + ATA_REG_STATUS);
    if (status == 0)
        return -1;

    if (ata_wait_bsy(base) < 0)
        return -1;

    uint8_t cl = inb(base + ATA_REG_LBA1);
    uint8_t ch = inb(base + ATA_REG_LBA2);
    if (cl != 0 || ch != 0)
        return -1;

    if (ata_wait_drq(base) < 0)
        return -1;

    uint16_t identify[256];
    for (int i = 0; i < 256; i++)
        identify[i] = inw(base + ATA_REG_DATA);

    return 0;
}

void ata_init()
{
    outb(ATA_PRIMARY_CTRL,   0x00);
    outb(ATA_SECONDARY_CTRL, 0x00);

    for (int b = 0; b < 2; b++)
    {
        for (int d = 0; d < 2; d++)
        {
            if (ata_probe(ata_bases[b], ata_drives[d]) == 0)
            {
                ata_base  = ata_bases[b];
                ata_drive = ata_drives[d];
                ata_ready = 1;
                return;
            }
        }
    }
}

int ata_read(uint32_t lba, uint8_t count, void *buf)
{
    if (!ata_ready)
        return -1;

    if (ata_wait_bsy(ata_base) < 0)
        return -1;

    outb(ata_base + ATA_REG_DRIVE,    ata_drive | 0x40 | ((lba >> 24) & 0x0F));
    ata_400ns_delay(ata_base);
    outb(ata_base + ATA_REG_SECCOUNT, count);
    outb(ata_base + ATA_REG_LBA0,     (uint8_t)(lba & 0xFF));
    outb(ata_base + ATA_REG_LBA1,     (uint8_t)((lba >> 8) & 0xFF));
    outb(ata_base + ATA_REG_LBA2,     (uint8_t)((lba >> 16) & 0xFF));
    outb(ata_base + ATA_REG_COMMAND,  ATA_CMD_READ_SECTORS);

    uint16_t *ptr = (uint16_t *)buf;
    for (int s = 0; s < count; s++)
    {
        if (ata_wait_bsy(ata_base) < 0)
            return -1;
        if (ata_wait_drq(ata_base) < 0)
            return -1;
        for (int i = 0; i < 256; i++)
            ptr[i] = inw(ata_base + ATA_REG_DATA);
        ptr += 256;
    }
    return 0;
}

int ata_write(uint32_t lba, uint8_t count, const void *buf)
{
    if (!ata_ready)
        return -1;

    if (ata_wait_bsy(ata_base) < 0)
        return -1;

    outb(ata_base + ATA_REG_DRIVE,    ata_drive | 0x40 | ((lba >> 24) & 0x0F));
    ata_400ns_delay(ata_base);
    outb(ata_base + ATA_REG_SECCOUNT, count);
    outb(ata_base + ATA_REG_LBA0,     (uint8_t)(lba & 0xFF));
    outb(ata_base + ATA_REG_LBA1,     (uint8_t)((lba >> 8) & 0xFF));
    outb(ata_base + ATA_REG_LBA2,     (uint8_t)((lba >> 16) & 0xFF));
    outb(ata_base + ATA_REG_COMMAND,  ATA_CMD_WRITE_SECTORS);

    const uint16_t *ptr = (const uint16_t *)buf;
    for (int s = 0; s < count; s++)
    {
        if (ata_wait_bsy(ata_base) < 0)
            return -1;
        if (ata_wait_drq(ata_base) < 0)
            return -1;
        for (int i = 0; i < 256; i++)
            outw(ata_base + ATA_REG_DATA, ptr[i]);
        ptr += 256;

        outb(ata_base + ATA_REG_COMMAND, 0xE7);
        ata_400ns_delay(ata_base);
        if (ata_wait_bsy(ata_base) < 0)
            return -1;
    }
    return 0;
}

int ata_identify(uint16_t *buf)
{
    if (!ata_ready)
        return -1;
    outb(ata_base + ATA_REG_DRIVE,   ata_drive);
    ata_400ns_delay(ata_base);
    outb(ata_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_400ns_delay(ata_base);
    if (ata_wait_bsy(ata_base) < 0)
    {
        return -1;
    }
    if (ata_wait_drq(ata_base) < 0)
    {
        return -1;
    }
    for (int i = 0; i < 256; i++)
    {
        buf[i] = inw(ata_base + ATA_REG_DATA);
    }
    return 0;
}

int ata_is_ready()
{
    return ata_ready;
}