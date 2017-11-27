#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int
dma_result(const char* filename, const char* result)
{
    int fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
        printf("Failed to open dev mem\n");
        return -1;
    }
    void* ptr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xD0000000);
    if (ptr == MAP_FAILED) {
        printf("Mmap failed!\n");
        return -1;
    }

    char buffer[4096];
    for (int i = 0; i < 4096; ++i)
        buffer[i] = '\0';
    strcpy(buffer, filename);
    strcat(buffer, "\n");
    strcat(buffer, result);

    //memcpy(ptr, buffer, 4096);
    strcpy((char*)ptr, buffer);
    fsync(fd);
}
