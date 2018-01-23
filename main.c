#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "version.h"
#include "analysis.h"

static struct option long_options[] =
{
    { "probe",   no_argument, NULL, 'p'},
    { "output",  required_argument, NULL, 'o'},
    { "help",    no_argument,       NULL, 'h'},
    { "verbose", required_argument, NULL, 'v'},
};

static char options[] =
"\t-o analysis output file, defaults to stdout\n"
"\t-p print information regarding this stream\n"
"\t-v print more info\n"
"\t-h print this message and exit\n";

void usage()
{
    fprintf(stderr, "flv_analysis, version %s\n", FLV_ANALYSIS_VERSION);
    fprintf(stderr, "Analysis flv format file\n");
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "flv_analysis [options] <input bitstream>\noptions:\n%s\n", options);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    int ret = 0;

    int opt_verbose = 1;
    int opt_probe = 0;
    int c = 0;
    int opt_index = 0;

    extern int iflv_fd;
    extern int oanls_fd;

    extern char* optarg;
    extern int   optind;
    while ((c = getopt_long(argc, argv, "o:p:hv", long_options, &opt_index)) != -1) {
        switch (c) {
            case 'o':
                oanls_fd = open(optarg, O_WRONLY|O_TRUNC|O_CREAT, 0666);
                break;
            case 'p':
                opt_probe = 1;
                opt_verbose = 0;
                break;
            case 'v':
                opt_verbose = atoi(optarg);
                break;
            case 'h':
            default:
                usage();
                return EXIT_FAILURE;
        }
    }

    if (oanls_fd == -1) {
        fprintf(stderr, "open analysis file failed: %s(%d)\n", strerror(errno), errno);
        usage();
        return EXIT_FAILURE;
    }

    if ((iflv_fd = open(argv[optind], O_RDONLY)) == -1) {
        fprintf(stderr, "open input flv file(%s) failed: %s(%d)\n", argv[optind], strerror(errno), errno);
        usage();
        return EXIT_FAILURE;
    }

    ret = analysis();

    return ret;
}
