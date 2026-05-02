#include "shell.h"
#include "types.h"
#include "../drivers/ext2.h"
#include "../lib/string.h"
#include "../lib/kstd.h"
#include "../cpu/cpu.h"

#define MAX_INPUT   128
#define MAX_ARGS    8
#define MAX_PATH    256

uint32_t multiboot_mem_lower = 0;
uint32_t multiboot_mem_upper = 0;

char cwd[MAX_PATH] = "/";

void build_path(const char *arg, char *out)
{
    if (arg[0] == '/')
    {
        strncpy(out, arg, MAX_PATH - 1);
        out[MAX_PATH - 1] = '\0';
        return;
    }
    if (cwd[1] == '\0')
    {
        out[0] = '/';
        strncpy(out + 1, arg, MAX_PATH - 2);
        out[MAX_PATH - 1] = '\0';
    }
    else
    {
        strncpy(out, cwd, MAX_PATH - 1);
        uint32_t len = strlen(out);
        if (len < MAX_PATH - 2)
        {
            out[len]     = '/';
            out[len + 1] = '\0';
            strncpy(out + len + 1, arg, MAX_PATH - len - 2);
        }
        out[MAX_PATH - 1] = '\0';
    }
}

int parse_args(char *input, char *argv[], int max_args)
{
    int argc = 0;
    char *p  = input;
    while (*p && argc < max_args)
    {
        while (*p == ' ')
            p++;
        if (*p == '\0')
            break;
        argv[argc++] = p;
        while (*p && *p != ' ')
            p++;
        if (*p == ' ')
            *p++ = '\0';
    }
    return argc;
}

uint cmd_ls(const char *path, uint line)
{
    static char entries[64][MAX_PATH];
    static char full[MAX_PATH];
    
    int count = ext2_list_dir(path, entries, 64);

    if (count < 0)
    {
        line = vga_write("ls: directory not found", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    for (int i = 0; i < count; i++)
    {
        if (strcmp(entries[i], ".") == 0 || strcmp(entries[i], "..") == 0)
            continue;
        build_path(entries[i], full);
        uchar color = ext2_is_dir(full) ? CYAN : WHITE;
        line = vga_putc(' ', WHITE, line);
        for (int j = 0; entries[i][j]; j++)
            line = vga_putc(entries[i][j], color, line);
        if (ext2_is_dir(full))
            line = vga_putc('/', CYAN, line);
        line = vga_putc('\n', WHITE, line);
    }
    return line;
}

uint cmd_cd(const char *arg, uint line)
{
    if (!arg || arg[0] == '\0')
    {
        strncpy(cwd, "/", MAX_PATH);
        return line;
    }
    if (strcmp(arg, "..") == 0)
    {
        if (cwd[1] == '\0')
            return line;
        int i = strlen(cwd) - 1;
        while (i > 0 && cwd[i] != '/')
            i--;
        if (i == 0)
            cwd[1] = '\0';
        else
            cwd[i] = '\0';
        return line;
    }
    char path[MAX_PATH];
    build_path(arg, path);
    if (!ext2_is_dir(path))
    {
        line = vga_write("cd: directory not found", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    strncpy(cwd, path, MAX_PATH - 1);
    cwd[MAX_PATH - 1] = '\0';
    return line;
}

uint cmd_fetch(uint line)
{
    line = vga_write("PXK | v2", line, WHITE);
    line = vga_write("Programmed by AlexXela", line, GREY);
    cpu_print_info(line);
    line++;
    return line;
}

uint cmd_mkdir(const char *arg, uint line)
{
    if (!arg || arg[0] == '\0')
    {
        line = vga_write("mkdir: no name", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    char path[MAX_PATH];
    build_path(arg, path);
    if (ext2_mkdir(path) < 0)
    {
        line = vga_write("mkdir: error", line, RED);
        line = vga_putc('\n', WHITE, line);
    }
    return line;
}

uint cmd_cat(const char *arg, uint line)
{
    if (!arg || arg[0] == '\0')
    {
        line = vga_write("cat: no name file", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    char path[MAX_PATH];
    build_path(arg, path);

    char buf[4096];
    int len = ext2_read_file(path, buf, sizeof(buf) - 1);
    if (len < 0)
    {
        line = vga_write("cat: file not found", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    buf[len] = '\0';
    for (int i = 0; i < len; i++)
        line = vga_putc(buf[i], WHITE, line);
    line = vga_putc('\n', WHITE, line);
    return line;
}

uint cmd_touch(const char *arg, uint line)
{
    if (!arg || arg[0] == '\0')
    {
        line = vga_write("touch: no name file", line, RED);
        line = vga_putc('\n', WHITE, line);
        return line;
    }
    char path[MAX_PATH];
    build_path(arg, path);
    if (ext2_create_file(path) < 0)
    {
        line = vga_write("touch: error", line, RED);
        line = vga_putc('\n', WHITE, line);
    }
    return line;
}

uint cmd_pwd(uint line)
{
    line = vga_write(cwd, line, WHITE);
    line = vga_putc('\n', WHITE, line);
    return line;
}

uint cmd_help(uint line)
{
    line = vga_write("comands: ls  cd  mkdir  cat  touch  pwd  help  clear  fastfetch", line, YELLOW);
    line = vga_putc('\n', WHITE, line);
    return line;
}

void build_prompt(char *out)
{
    strcpy(out, "root:");
    strcat(out, cwd);
    strcat(out, " $ ");
}

void shell_run(uint line)
{
    char  input[MAX_INPUT];
    char  prompt[MAX_PATH + 16];
    char *argv[MAX_ARGS];

    while (1)
    {
        build_prompt(prompt);
        line = vga_scan(input, MAX_INPUT, line, prompt);

        int argc = parse_args(input, argv, MAX_ARGS);
        if (argc == 0)
            continue;

        char *cmd = argv[0];

        if (strcmp(cmd, "ls") == 0)
        {
            char path[MAX_PATH];
            if (argc > 1)
                build_path(argv[1], path);
            else
                strncpy(path, cwd, MAX_PATH);
            line = cmd_ls(path, line);
        }
        else if (strcmp(cmd, "cd") == 0) line = cmd_cd(argc > 1 ? argv[1] : 0, line);
        else if (strcmp(cmd, "mkdir") == 0) line = cmd_mkdir(argc > 1 ? argv[1] : 0, line);
        else if (strcmp(cmd, "cat") == 0) line = cmd_cat(argc > 1 ? argv[1] : 0, line);
        else if (strcmp(cmd, "touch") == 0) line = cmd_touch(argc > 1 ? argv[1] : 0, line);
        else if (strcmp(cmd, "pwd") == 0) line = cmd_pwd(line);
        else if (strcmp(cmd, "fastfetch") == 0) cpu_print_info(&line);
        else if (strcmp(cmd, "help") == 0) line = cmd_help(line);
        else if (strcmp(cmd, "clear") == 0)
        {
            vga_clean();
            line = 0;
        }
        else
        {
            line = vga_write(cmd, line, RED);
            line = vga_write(": command not found!", line, RED);
            line = vga_putc('\n', WHITE, line);
        }
    }
}