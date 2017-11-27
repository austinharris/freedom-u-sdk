#include <getopt.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


static struct option const long_opts[] =
{
    {"load", no_argument, NULL, 'l'},
    {"reset", no_argument, NULL, 'r'},
    {"result", no_argument, NULL, 'x'},
    {"file", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void
dma_access(uint64_t addr, const char* filename, int writeTo)
{
    uint64_t size;
    char *devicename = "/dev/xdma/card0/h2c0";
    int rc;
    char *buffer = NULL;
    char *allocated = NULL;

    if (writeTo) {
        //calculate size of file
        struct stat st;
        stat(filename, &st);
        size = st.st_size;
    }
    else {
        //default to allowing 1 4KB page of results to read out
        size = 4096;
    }

    posix_memalign((void **)&allocated, 4096/*alignment*/, size + 4096);
    assert(allocated);
    buffer = allocated;
    printf("host memory buffer = %p\n", buffer);

    int file_fd = -1;
    int fpga_fd = -1;
    if (writeTo)
        fpga_fd = open(devicename, O_RDWR);
    else
        fpga_fd = open(devicename, O_RDWR | O_NONBLOCK);
    assert(fpga_fd >= 0);

    if (filename) {
        if (writeTo)
            //writing to FPGA memory so only need to read the file to write to memory
            file_fd = open(filename, O_RDONLY);
        else
            //reading from FPGA memory so need to write the result to this file
            file_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0666);
        assert(file_fd >= 0);
    }
    if (file_fd >= 0) {
        rc = read(file_fd, buffer, size);
        if (rc != size) perror("read(file_fd)");
        assert (rc == size);
    }
    lseek(fpga_fd, addr, SEEK_SET);
    if (writeTo) {
        rc = write(fpga_fd, buffer, size);
    }
    else {
        rc = read(fpga_fd, buffer, size);
        if (file_fd >= 0) {
            rc = write(file_fd, buffer, size);
            assert(rc == size);
        }
    }
    close(fpga_fd);

}

void
usage(const char* name)
{
    int i = 0;
    printf("%s\n\n", name);
    printf("usage: %s [OPTIONS]\n\n", name);
    printf("Load Linux bbl image, reset Rocket, or read Rocket result output.\n\n");

    printf("  -%c (--%s) load bbl image and reset Rocket)\n", long_opts[i].val, long_opts[i].name); ++i;
    printf("  -%c (--%s) reset already running  Rocket, reloading bbl image)\n", long_opts[i].val, long_opts[i].name); ++i;
    printf("  -%c (--%s) read result from Rocket and write to output file)\n", long_opts[i].val, long_opts[i].name); ++i;
    printf("  -%c (--%s) filename (bbl or output file)\n", long_opts[i].val, long_opts[i].name); ++i;
    printf("  -%c (--%s) print usage help and exit\n", long_opts[i].val, long_opts[i].name); ++i;
}

void
load(const char* filename)
{
    printf("Loading %s into Rocket Memory!\n", filename);
}

void
reset(const char* filename)
{
    printf("Resetting and loading %s into Rocket Memory!\n", filename);
}

void
read_result(const char* filename)
{
    printf("Reading result from Rocket memory into  %s !\n", filename);
}

int
main(int argc, char* argv[])
{
    int cmd_opt;
    char* filename = NULL;
    enum operation {LOAD, RESET, RESULT, NONE} op = NONE;
    while ((cmd_opt = getopt_long(argc, argv, "lrxc:f:", long_opts, NULL)) != -1)
    {
        switch (cmd_opt)
        {
          case 0:
            /* long option */
            break;
          case 'l':
            op= LOAD;
            break;
          case 'r':
            op= RESET;
            break;
          case 'x':
            op= RESULT;
            break;
          case 'f':
            filename = strdup(optarg);
            break;
            /* print usage help and exit */
          case 'h':
          default:
            usage(argv[0]);
            exit(0);
            break;
        }
    }

    switch(op)
    {
      case LOAD: load(filename); break;
      case RESET: reset(filename); break;
      case RESULT: read_result(filename); break;
      default:
        usage(argv[0]);
        exit(-1);
    }
}
