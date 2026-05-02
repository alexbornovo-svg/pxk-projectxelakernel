#include "../lib/xelagraph.h"
#include "../lib/string.h"
#include "../drivers/ext2.h"
#include "../programs/shell_graphic.h"
#include "../misc/logo1616.h"
#include "../lib/kstd.h"

#define MAX_INPUT  128
#define MAX_ARGS   8
#define MAX_PATH   256

static char gcwd[MAX_PATH] = "/";

static void g_build_path(const char *arg, char *out)
{
    if (arg[0] == '/') {
        strncpy(out, arg, MAX_PATH - 1);
        out[MAX_PATH - 1] = '\0';
        return;
    }
    if (gcwd[1] == '\0') {
        out[0] = '/';
        strncpy(out + 1, arg, MAX_PATH - 2);
        out[MAX_PATH - 1] = '\0';
    } else {
        strncpy(out, gcwd, MAX_PATH - 1);
        uint32_t len = strlen(out);
        if (len < MAX_PATH - 2) {
            out[len]     = '/';
            out[len + 1] = '\0';
            strncpy(out + len + 1, arg, MAX_PATH - len - 2);
        }
        out[MAX_PATH - 1] = '\0';
    }
}

static int g_parse_args(char *input, char *argv[], int max_args)
{
    int argc = 0;
    char *p = input;
    while (*p && argc < max_args) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}

static void g_cmd_ls(const char *path, uint line)
{
    static char entries[64][MAX_PATH];
    static char full[MAX_PATH];
    int count = ext2_list_dir(path, entries, 64);
    if (count < 0) { g_write("ls: not found", line, 3); return; }
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i], ".") == 0 || strcmp(entries[i], "..") == 0) continue;
        g_build_path(entries[i], full);
        uint col = ext2_is_dir(full) ? 6 : 2;
        g_putc(' ', 2, line);
        for (int j = 0; entries[i][j]; j++)
            g_putc(entries[i][j], col, line);
        if (ext2_is_dir(full)) g_putc('/', 6, line);
        g_putc('\n', 2, line);
    }
}

static void g_cmd_cd(const char *arg)
{
    if (!arg || arg[0] == '\0') { strncpy(gcwd, "/", MAX_PATH); return; }
    if (strcmp(arg, "..") == 0) {
        if (gcwd[1] == '\0') return;
        int i = strlen(gcwd) - 1;
        while (i > 0 && gcwd[i] != '/') i--;
        if (i == 0) gcwd[1] = '\0'; else gcwd[i] = '\0';
        return;
    }
    char path[MAX_PATH];
    g_build_path(arg, path);
    if (!ext2_is_dir(path)) { g_write("cd: not found", 0, 3); return; }
    strncpy(gcwd, path, MAX_PATH - 1);
    gcwd[MAX_PATH - 1] = '\0';
}

static void g_cmd_cat(const char *arg, uint line)
{
    if (!arg || arg[0] == '\0') { g_write("cat: no file", line, 3); return; }
    char path[MAX_PATH];
    g_build_path(arg, path);
    char buf[4096];
    int len = ext2_read_file(path, buf, sizeof(buf) - 1);
    if (len < 0) { g_write("cat: not found", line, 3); return; }
    buf[len] = '\0';
    for (int i = 0; i < len; i++) g_putc(buf[i], 2, line);
    g_putc('\n', 2, line);
}

static void g_build_prompt(char *out)
{
    strcpy(out, "root:");
    strcat(out, gcwd);
    strcat(out, " $ ");
}

void shell_graphic_run(multiboot_info_t *mbi)
{
    fb_init(mbi);
    clearscreen(0x111111);
    topbar_set_cwd(gcwd);
    topbar_refresh();

    uint32_t w = fb_width();
    drawimage_indexed_scaled(w - 20, 2, 16, 16, 1, pxklogo_map, pxklogo_palette);

    // offset cursore sotto la topbar
    cursor_set(0, 2);

    g_write("PXK2 Graphic Shell - type 'help'", 0, 4);
    g_write("", 0, 2);

    char input[MAX_INPUT];
    char prompt[MAX_PATH + 16];
    char *argv[MAX_ARGS];
    uint line = 0;

    while (1)
    {
        topbar_refresh();
        g_build_prompt(prompt);
        line = g_scan(input, MAX_INPUT, line, prompt);

        int argc = g_parse_args(input, argv, MAX_ARGS);
        if (argc == 0) continue;

        char *cmd = argv[0];

        if (strcmp(cmd, "ls") == 0) {
            char path[MAX_PATH];
            if (argc > 1) g_build_path(argv[1], path);
            else strncpy(path, gcwd, MAX_PATH);
            g_cmd_ls(path, line);
        }
        else if (strcmp(cmd, "cd") == 0) {
            g_cmd_cd(argc > 1 ? argv[1] : 0);
            topbar_set_cwd(gcwd);
        }
        else if (strcmp(cmd, "cat") == 0)  g_cmd_cat(argc > 1 ? argv[1] : 0, line);
        else if (strcmp(cmd, "pwd") == 0)  g_write(gcwd, line, 2);
        else if (strcmp(cmd, "clear") == 0) {
            clearscreen(0x111111);
            topbar_refresh();
            cursor_set(0, 2);
            line = 0;
        }
        else if (strcmp(cmd, "help") == 0)
            g_write("commands: ls  cd  cat  pwd  clear  help", line, 7);
        else {
            g_write(cmd, line, 3);
            g_write(": command not found", line, 3);
        }
    }
}