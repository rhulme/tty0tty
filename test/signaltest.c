/* Quick and dirty test program to check that TIOCMIWAIT works correctly */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define SLEEP_TIME   (1000 * 1000)

int listen(const char* prefix, int fd)
{
    int status;
    int ret = 0;

    if(prefix == 0)
        prefix = "";

    while(ret == 0)
    {
        if((ret = ioctl (fd, TIOCMIWAIT, (TIOCM_CD|TIOCM_DSR|TIOCM_RI|TIOCM_CTS)) == -1))
            fprintf(stderr, "%s: TIOCMIWAIT failed: %s (%d)\n", prefix, strerror(errno), errno);
        else
        {
            if((ret = ioctl (fd, TIOCMGET, &status)) == -1)
                fprintf(stderr, "%s: TIOCMGET failed: %s (%d)\n", prefix, strerror(errno), errno);
            else
            {
                printf("  %s: CTS: %d\n", prefix, !!(status & TIOCM_CTS));
                printf("  %s: DSR: %d\n", prefix, !!(status & TIOCM_DSR));
                printf("  %s: CD:  %d\n", prefix, !!(status & TIOCM_CD));
                printf("  %s: RI:  %d\n", prefix, !!(status & TIOCM_RI));
                printf("\n");
            }
        }
    }
    
    return ret;
}

int setLevel(const char* prefix, int fd, int status)
{
    int ret;

    if((ret = ioctl (fd, TIOCMSET, &status)) == -1)
        fprintf(stderr, "%s: TIOCMSET failed: %s (%d)\n", prefix, strerror(errno), errno);

    usleep(SLEEP_TIME);
    
    return ret;
}

int trigger(const char* prefix, int fd)
{
    int status = 0;
    int ret = 0;

    if(prefix == 0)
        prefix = "";
    
    while(ret == 0)
    {
        status ^= TIOCM_RTS;
        printf("%s: RTS: %d\n", prefix, !!(status & TIOCM_RTS));
        ret = setLevel(prefix, fd, status);

        if(ret == 0)
        {
            status ^= TIOCM_DTR;
            printf("%s: DTR: %d\n", prefix, !!(status & TIOCM_DTR));
            ret = setLevel(prefix, fd, status);
        }
    }
    
    return ret;
}


int opentty(const char* filename)
{
    int fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd == -1)
        fprintf(stderr, "%s: Error opening %s - %s (%d)\n", filename, filename, strerror(errno), errno);
    

    return fd;
}

void printHelp()
{
    printf("signaltest\n");
    printf("\n");
    printf("Simple test to check whether TIOCMIWAIT works correctly\n");
    printf("\n");
    printf("Toggles RTS and DTR once per second on a base device (e.g. /dev/tnt0)\n");
    printf("while waiting on the shadow device (in that case /dev/tnt1) for the change.\n");
    printf("\n");
    printf("signaltest [-b <base device number>]\n");
    printf("\n");
    printf("<base device number> should be just a number (generally 0-9)\n");
    printf("\n");
}

int getBaseDevice(int argc, char** argv)
{
    int c;
    int base = 0;

    while(base == 0)
    {
        int option_index = 0;
        static struct option long_options[] = {{"help", no_argument, 0,  'h' },
                                               {0,      0,           0,  0 }
                                              };

        c = getopt_long(argc, argv, "hb:", long_options, &option_index);
        if (c == -1)
            break;

        switch(c)
        {
            case 'b':
                base = atoi(optarg);
                if(base < 0)
                {
                    fprintf(stderr, "Base number cannot be negative!\n");
                    printHelp();
                    base = -1;
                }
                
                break;

            default:
            case 'h':
                printHelp();
                base = -1;
                break;
        }
    }
    
    return base;
}

int main(int argc, char** argv)
{
    int ret = 0;
    int baseDevice = getBaseDevice(argc, argv);
    char base[256];
    char shadow[256];
    
    if(baseDevice >= 0)
    {
        snprintf(base, sizeof(base), "/dev/tnt%d", baseDevice);
        snprintf(shadow, sizeof(shadow), "/dev/tnt%d", baseDevice ^ 1);
        base[sizeof(base)-1] = '\0';
        shadow[sizeof(shadow)-1] = '\0';

        if(fork())
        {
            int fd = opentty(base);

            if(fd != -1)
                ret = trigger(base, fd);
        }
        else
        {
            int fd = opentty(shadow);

            if(fd != -1)
                ret = listen(shadow, fd);
        }
    }
    else
        ret = 1;

    return ret;
}
