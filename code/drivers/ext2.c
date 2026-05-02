#include "ext2.h"
#include "ata.h"
#include "../lib/string.h"
#include "../lib/kstd.h"

static ext2_fs_t fs;

static uint32_t block_to_lba(uint32_t block)
{
    return fs.lba_start + block * fs.sectors_per_block;
}

static int read_block(uint32_t block, void *buf)
{
    return ata_read(block_to_lba(block), fs.sectors_per_block, buf);
}

static int write_block(uint32_t block, const void *buf)
{
    return ata_write(block_to_lba(block), fs.sectors_per_block, buf);
}

static int read_inode(uint32_t inode_num, ext2_inode_t *out)
{
    uint32_t group    = (inode_num - 1) / fs.sb.s_inodes_per_group;
    uint32_t index    = (inode_num - 1) % fs.sb.s_inodes_per_group;
    uint32_t inode_size = fs.sb.s_rev_level >= 1 ? fs.sb.s_inode_size : 128;

    uint32_t gdt_block = (fs.sb.s_first_data_block + 1);
    uint8_t  gdt_buf[fs.block_size];
    if (read_block(gdt_block, gdt_buf) < 0)
        return -1;

    ext2_group_desc_t *gd = (ext2_group_desc_t *)(gdt_buf + group * sizeof(ext2_group_desc_t));

    uint32_t table_block  = gd->bg_inode_table;
    uint32_t byte_offset  = index * inode_size;
    uint32_t block_offset = byte_offset / fs.block_size;
    uint32_t inner_offset = byte_offset % fs.block_size;

    uint8_t block_buf[fs.block_size];
    if (read_block(table_block + block_offset, block_buf) < 0)
        return -1;

    memcpy(out, block_buf + inner_offset, sizeof(ext2_inode_t));
    return 0;
}

static int write_inode(uint32_t inode_num, ext2_inode_t *in)
{
    uint32_t group     = (inode_num - 1) / fs.sb.s_inodes_per_group;
    uint32_t index     = (inode_num - 1) % fs.sb.s_inodes_per_group;
    uint32_t inode_size = fs.sb.s_rev_level >= 1 ? fs.sb.s_inode_size : 128;

    uint32_t gdt_block = (fs.sb.s_first_data_block + 1);
    uint8_t  gdt_buf[fs.block_size];
    if (read_block(gdt_block, gdt_buf) < 0)
        return -1;

    ext2_group_desc_t *gd = (ext2_group_desc_t *)(gdt_buf + group * sizeof(ext2_group_desc_t));

    uint32_t table_block  = gd->bg_inode_table;
    uint32_t byte_offset  = index * inode_size;
    uint32_t block_offset = byte_offset / fs.block_size;
    uint32_t inner_offset = byte_offset % fs.block_size;

    uint8_t block_buf[fs.block_size];
    if (read_block(table_block + block_offset, block_buf) < 0)
        return -1;

    memcpy(block_buf + inner_offset, in, sizeof(ext2_inode_t));
    return write_block(table_block + block_offset, block_buf);
}

static int alloc_block(uint32_t group)
{
    uint32_t gdt_block = fs.sb.s_first_data_block + 1;
    uint8_t  gdt_buf[fs.block_size];
    if (read_block(gdt_block, gdt_buf) < 0)
        return -1;

    ext2_group_desc_t *gd = (ext2_group_desc_t *)(gdt_buf + group * sizeof(ext2_group_desc_t));
    if (gd->bg_free_blocks_count == 0)
        return -1;

    uint8_t bitmap[fs.block_size];
    if (read_block(gd->bg_block_bitmap, bitmap) < 0)
        return -1;

    for (uint32_t i = 0; i < fs.sb.s_blocks_per_group; i++) 
    {
        uint32_t byte = i / 8;
        uint32_t bit  = i % 8;
        if (!(bitmap[byte] & (1 << bit))) 
        {
            bitmap[byte] |= (1 << bit);
            write_block(gd->bg_block_bitmap, bitmap);
            gd->bg_free_blocks_count--;
            write_block(gdt_block, gdt_buf);
            fs.sb.s_free_blocks_count--;
            return group * fs.sb.s_blocks_per_group + i + fs.sb.s_first_data_block;
        }
    }
    return -1;
}

static int alloc_inode(uint32_t group)
{
    uint32_t gdt_block = fs.sb.s_first_data_block + 1;
    uint8_t  gdt_buf[fs.block_size];
    if (read_block(gdt_block, gdt_buf) < 0)
        return -1;

    ext2_group_desc_t *gd = (ext2_group_desc_t *)(gdt_buf + group * sizeof(ext2_group_desc_t));
    if (gd->bg_free_inodes_count == 0)
        return -1;

    uint8_t bitmap[fs.block_size];
    if (read_block(gd->bg_inode_bitmap, bitmap) < 0)
        return -1;

    for (uint32_t i = 0; i < fs.sb.s_inodes_per_group; i++) 
    {
        uint32_t byte = i / 8;
        uint32_t bit  = i % 8;
        if (!(bitmap[byte] & (1 << bit))) 
        {
            bitmap[byte] |= (1 << bit);
            write_block(gd->bg_inode_bitmap, bitmap);
            gd->bg_free_inodes_count--;
            write_block(gdt_block, gdt_buf);
            fs.sb.s_free_inodes_count--;
            return group * fs.sb.s_inodes_per_group + i + 1;
        }
    }
    return -1;
}

static int read_inode_block(ext2_inode_t *inode, uint32_t block_index, void *buf)
{
    if (block_index < 12) 
    {
        return read_block(inode->i_block[block_index], buf);
    }

    uint32_t ptrs_per_block = fs.block_size / 4;
    uint32_t indirect_buf[ptrs_per_block];

    if (block_index < 12 + ptrs_per_block) 
    {
        if (read_block(inode->i_block[12], indirect_buf) < 0)
            return -1;
        return read_block(indirect_buf[block_index - 12], buf);
    }

    uint32_t double_idx = block_index - 12 - ptrs_per_block;
    uint32_t di_buf[ptrs_per_block];
    if (read_block(inode->i_block[13], di_buf) < 0)
        return -1;
    uint32_t si_buf[ptrs_per_block];
    if (read_block(di_buf[double_idx / ptrs_per_block], si_buf) < 0)
        return -1;
    return read_block(si_buf[double_idx % ptrs_per_block], buf);
}

static int write_inode_block(ext2_inode_t *inode, uint32_t inode_num,
                             uint32_t block_index, void *buf)
{
    uint32_t ptrs_per_block = fs.block_size / 4;

    if (block_index < 12) 
    {
        if (inode->i_block[block_index] == 0) 
        {
            int nb = alloc_block(0);
            if (nb < 0)
                return -1;
            inode->i_block[block_index] = nb;
            inode->i_blocks += fs.sectors_per_block;
            write_inode(inode_num, inode);
        }
        return write_block(inode->i_block[block_index], buf);
    }

    if (block_index < 12 + ptrs_per_block) 
    {
        uint32_t indirect_buf[ptrs_per_block];
        if (inode->i_block[12] == 0) 
        {
            int nb = alloc_block(0);
            if (nb < 0)
                return -1;
            inode->i_block[12] = nb;
            inode->i_blocks += fs.sectors_per_block;
            write_inode(inode_num, inode);
            memset(indirect_buf, 0, fs.block_size);
        } 
        else 
        {
            read_block(inode->i_block[12], indirect_buf);
        }
        uint32_t idx = block_index - 12;
        if (indirect_buf[idx] == 0) 
        {
            int nb = alloc_block(0);
            if (nb < 0)
                return -1;
            indirect_buf[idx] = nb;
            inode->i_blocks  += fs.sectors_per_block;
            write_block(inode->i_block[12], indirect_buf);
            write_inode(inode_num, inode);
        }
        return write_block(indirect_buf[idx], buf);
    }

    return -1;
}

static uint32_t lookup_inode(const char *path)
{
    if (path[0] != '/')
        return 0;

    uint32_t current_inode = EXT2_ROOT_INODE;

    if (path[1] == '\0')
        return current_inode;

    char tmp[EXT2_MAX_PATH];
    strncpy(tmp, path + 1, EXT2_MAX_PATH - 1);
    tmp[EXT2_MAX_PATH - 1] = '\0';

    char *token = tmp;
    while (token && *token) 
    {
        char *slash = strchr(token, '/');
        if (slash)
            *slash = '\0';

        ext2_inode_t inode;
        if (read_inode(current_inode, &inode) < 0)
            return 0;

        if (!(inode.i_mode & EXT2_S_IFDIR))
            return 0;

        uint32_t found = 0;
        uint32_t blocks = (inode.i_size + fs.block_size - 1) / fs.block_size;
        uint8_t  block_buf[fs.block_size];

        for (uint32_t b = 0; b < blocks && !found; b++) 
        {
            if (read_inode_block(&inode, b, block_buf) < 0)
                break;
            uint32_t offset = 0;
            while (offset < fs.block_size) 
            {
                ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(block_buf + offset);
                if (entry->inode == 0 || entry->rec_len == 0)
                    break;
                if (entry->name_len == strlen(token) &&
                    strncmp(entry->name, token, entry->name_len) == 0) 
                {
                    found = entry->inode;
                    break;
                }
                offset += entry->rec_len;
            }
        }

        if (!found)
            return 0;

        current_inode = found;
        token = slash ? slash + 1 : 0;
    }
    return current_inode;
}

int ext2_init(uint32_t lba_start)
{
    fs.lba_start = lba_start;

    uint8_t sb_buf[1024];
    if (ata_read(lba_start + 2, 2, sb_buf) < 0)
        return -1;

    memcpy(&fs.sb, sb_buf, sizeof(ext2_superblock_t));

    if (fs.sb.s_magic != EXT2_MAGIC)
        return -1;

    fs.block_size        = EXT2_BLOCK_SIZE_BASE << fs.sb.s_log_block_size;
    fs.sectors_per_block = fs.block_size / 512;
    return 0;
}

int ext2_read_file(const char *path, void *buf, uint32_t max_len)
{
    uint32_t ino = lookup_inode(path);
    if (!ino)
        return -1;

    ext2_inode_t inode;
    if (read_inode(ino, &inode) < 0)
        return -1;

    if (!(inode.i_mode & EXT2_S_IFREG))
        return -1;

    uint32_t size = inode.i_size < max_len ? inode.i_size : max_len;
    uint32_t blocks = (size + fs.block_size - 1) / fs.block_size;
    uint8_t  block_buf[fs.block_size];
    uint8_t  *out = (uint8_t *)buf;
    uint32_t remaining = size;

    for (uint32_t b = 0; b < blocks; b++) 
    {
        if (read_inode_block(&inode, b, block_buf) < 0)
            return -1;
        uint32_t chunk = remaining < fs.block_size ? remaining : fs.block_size;
        memcpy(out, block_buf, chunk);
        out       += chunk;
        remaining -= chunk;
    }
    return size;
}

int ext2_list_dir(const char *path, char out[][EXT2_MAX_PATH], int max_entries)
{
    uint32_t ino = lookup_inode(path);
    if (!ino)
        return -1;

    ext2_inode_t inode;
    if (read_inode(ino, &inode) < 0)
        return -1;

    if (!(inode.i_mode & EXT2_S_IFDIR))
        return -1;

    uint32_t blocks = (inode.i_size + fs.block_size - 1) / fs.block_size;
    uint8_t  block_buf[fs.block_size];
    int      count = 0;

    for (uint32_t b = 0; b < blocks && count < max_entries; b++) 
    {
        if (read_inode_block(&inode, b, block_buf) < 0)
            return -1;
        uint32_t offset = 0;
        while (offset < fs.block_size && count < max_entries) 
        {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(block_buf + offset);
            if (entry->inode == 0 || entry->rec_len == 0)
                break;
            strncpy(out[count], entry->name, entry->name_len);
            out[count][entry->name_len] = '\0';
            count++;
            offset += entry->rec_len;
        }
    }
    return count;
}

int ext2_is_dir(const char *path)
{
    uint32_t ino = lookup_inode(path);
    if (!ino)
        return 0;
    ext2_inode_t inode;
    if (read_inode(ino, &inode) < 0)
        return 0;
    return (inode.i_mode & EXT2_S_IFDIR) ? 1 : 0;
}

int ext2_write_file(const char *path, const void *buf, uint32_t len)
{
    uint32_t ino = lookup_inode(path);
    if (!ino)
        return -1;

    ext2_inode_t inode;
    if (read_inode(ino, &inode) < 0)
        return -1;

    if (!(inode.i_mode & EXT2_S_IFREG))
        return -1;

    const uint8_t *src       = (const uint8_t *)buf;
    uint32_t       remaining  = len;
    uint32_t       block_index = 0;
    uint8_t        block_buf[fs.block_size];

    while (remaining > 0) 
    {
        uint32_t chunk = remaining < fs.block_size ? remaining : fs.block_size;
        memset(block_buf, 0, fs.block_size);
        memcpy(block_buf, src, chunk);
        if (write_inode_block(&inode, ino, block_index, block_buf) < 0)
            return -1;
        src         += chunk;
        remaining   -= chunk;
        block_index++;
    }

    inode.i_size = len;
    write_inode(ino, &inode);
    return len;
}

static int dir_add_entry(uint32_t dir_ino, uint32_t new_ino,
                         const char *name, uint8_t file_type)
{
    ext2_inode_t inode;
    if (read_inode(dir_ino, &inode) < 0)
        return -1;

    uint8_t  name_len     = strlen(name);
    uint16_t needed       = (8 + name_len + 3) & ~3;
    uint32_t blocks       = (inode.i_size + fs.block_size - 1) / fs.block_size;
    uint8_t  block_buf[fs.block_size];

    for (uint32_t b = 0; b < blocks; b++) 
    {
        if (read_inode_block(&inode, b, block_buf) < 0)
            return -1;
        uint32_t offset = 0;
        while (offset < fs.block_size) 
        {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(block_buf + offset);
            if (entry->rec_len == 0)
                break;
            uint16_t real_len = (8 + entry->name_len + 3) & ~3;
            uint16_t free     = entry->rec_len - real_len;
            if (free >= needed) 
            {
                uint32_t old_rec_len  = entry->rec_len;
                entry->rec_len        = real_len;
                ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)(block_buf + offset + real_len);
                new_entry->inode     = new_ino;
                new_entry->rec_len   = old_rec_len - real_len;
                new_entry->name_len  = name_len;
                new_entry->file_type = file_type;
                memcpy(new_entry->name, name, name_len);
                return write_inode_block(&inode, dir_ino, b, block_buf);
            }
            offset += entry->rec_len;
        }
    }

    memset(block_buf, 0, fs.block_size);
    ext2_dir_entry_t *entry = (ext2_dir_entry_t *)block_buf;
    entry->inode     = new_ino;
    entry->rec_len   = fs.block_size;
    entry->name_len  = name_len;
    entry->file_type = file_type;
    memcpy(entry->name, name, name_len);

    if (write_inode_block(&inode, dir_ino, blocks, block_buf) < 0)
        return -1;

    inode.i_size += fs.block_size;
    return write_inode(dir_ino, &inode);
}

static void split_path(const char *path, char *parent, char *name)
{
    strncpy(parent, path, EXT2_MAX_PATH - 1);
    parent[EXT2_MAX_PATH - 1] = '\0';
    char *last = 0;
    for (int i = 0; parent[i]; i++)
        if (parent[i] == '/')
            last = parent + i;
    if (!last || last == parent) 
    {
        strcpy(parent, "/");
        strncpy(name, path + 1, EXT2_MAX_PATH - 1);
    } 
    else 
    {
        *last = '\0';
        strncpy(name, last + 1, EXT2_MAX_PATH - 1);
    }
    name[EXT2_MAX_PATH - 1] = '\0';
}

int ext2_create_file(const char *path)
{
    char parent[EXT2_MAX_PATH];
    char name[EXT2_MAX_PATH];
    split_path(path, parent, name);

    uint32_t parent_ino = lookup_inode(parent);
    if (!parent_ino)
        return -1;

    int new_ino = alloc_inode(0);
    if (new_ino < 0)
        return -1;

    ext2_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    inode.i_mode       = EXT2_S_IFREG | 0644;
    inode.i_links_count = 1;
    write_inode(new_ino, &inode);

    return dir_add_entry(parent_ino, new_ino, name, EXT2_FT_REG_FILE);
}

int ext2_mkdir(const char *path)
{
    char parent[EXT2_MAX_PATH];
    char name[EXT2_MAX_PATH];
    split_path(path, parent, name);

    uint32_t parent_ino = lookup_inode(parent);
    if (!parent_ino)
        return -1;

    int new_ino = alloc_inode(0);
    if (new_ino < 0)
        return -1;

    int data_block = alloc_block(0);
    if (data_block < 0)
        return -1;

    uint8_t block_buf[fs.block_size];
    memset(block_buf, 0, fs.block_size);

    ext2_dir_entry_t *dot = (ext2_dir_entry_t *)block_buf;
    dot->inode     = new_ino;
    dot->rec_len   = 12;
    dot->name_len  = 1;
    dot->file_type = EXT2_FT_DIR;
    dot->name[0]   = '.';

    ext2_dir_entry_t *dotdot = (ext2_dir_entry_t *)(block_buf + 12);
    dotdot->inode     = parent_ino;
    dotdot->rec_len   = fs.block_size - 12;
    dotdot->name_len  = 2;
    dotdot->file_type = EXT2_FT_DIR;
    dotdot->name[0]   = '.';
    dotdot->name[1]   = '.';

    write_block(data_block, block_buf);

    ext2_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    inode.i_mode        = EXT2_S_IFDIR | 0755;
    inode.i_links_count = 2;
    inode.i_size        = fs.block_size;
    inode.i_blocks      = fs.sectors_per_block;
    inode.i_block[0]    = data_block;
    write_inode(new_ino, &inode);

    return dir_add_entry(parent_ino, new_ino, name, EXT2_FT_DIR);
}