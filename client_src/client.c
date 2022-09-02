#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define CHANNELS 8

#define DEF_BUF_SIZE 256

#define DISABLED 0
#define ENABLED 1

// validation macros
#define FREQ_CHECK(FREQ) (FREQ >= 1 && FREQ <= 1000)
#define SAMPLES_CHECK(SAMPLES) (SAMPLES >= 1 && SAMPLES <= 10000)
#define MODE_CHECK(MODE) (MODE == 0 || MODE == 1)
#define ENABLED_CHANNEL_CHECK(CHANNEL) (CHANNEL == DISABLED || CHANNEL == ENABLED)

int output_fds[CHANNELS];
int arduino;

void open_output_fds(int output_fds[CHANNELS])
{
    for (int i = 0; i < CHANNELS; ++i)
    {
        char out_path[24];
        snprintf(out_path, 24, "../outputs/out_ch%d.txt", i);
        int fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 644);
        if (fd < 0) // check opening errors
        {
            printf("Error opening output files! Missing: %s\n", out_path);
            exit(EXIT_FAILURE);
        }
        output_fds[i] = fd;
    }
    printf("File descriptors opened correctly!\n");
}

void close_output_fds(int output_fds[CHANNELS])
{
    for (int i = 0; i < CHANNELS; ++i)
    {
        int res = close(output_fds[i]);
        if (res == -1) // check closing errors
        {
            printf("Error closing output files!\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("File descriptors closed correctly!\n");
}

int open_arduino()
{
    printf("Looking for Arduino on serial port...\n");
    FILE *fp = popen("find /dev/cu.usbmodem*", "r"); // run subprocess
    char path[DEF_BUF_SIZE];
    if (fp == NULL)
    {
        printf("Arduino not found!\n");
        exit(EXIT_FAILURE);
    }
    fgets(path, sizeof(path), fp); // read subprocess output
    path[strcspn(path, "\n")] = 0; // remove newline from fgets() reading
    pclose(fp);

    if (path[0] != '/') // if output does not start with '/' -> find throws a error -> serial device not found
    {
        printf("Arduino not found!\n");
        exit(EXIT_FAILURE);
    }
    printf("Found Arduino!\n");

    int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC); // open file descriptor
    if (fd < 0)                                      // check opening errors
    {
        printf("Error opening Arduino connection!\n");
        exit(EXIT_FAILURE);
    }

    printf("Arduino connection open!\n");
    return fd;
}

void close_arduino(int arduino)
{
    int res = close(arduino);
    if (res == -1) // check closing errors
    {
        printf("Error closing Arduino connection!\n");
        exit(EXIT_FAILURE);
    }
    printf("Closed Arduino connection!\n");
}

int serial_set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        printf("Error from tcgetattr\n");
        return -1;
    }
    switch (speed)
    {
    case 19200:
        speed = B19200;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        printf("Cannot set baudrate %d\n", speed);
        return -1;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    cfmakeraw(&tty);
    // enable reading
    tty.c_cflag &= -(PARENB | PARODD); // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag = (tty.c_cflag & -CSIZE) | CS8; // 8-bit chars

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        printf("Error from tcsettatr\n");
        return -1;
    }
    printf("Arduino connection setup correctly!\n");
    return 0;
}

void get_channels(int channels[CHANNELS])
{
    int invalid_input;
    do
    {
        invalid_input = 0;
        char input[DEF_BUF_SIZE];
        printf("Insert enabled channels (with spacing) [8 values: {0: disabled, 1: enabled}]: ");
        scanf("%d %d %d %d %d %d %d %d", &channels[0], &channels[1], &channels[2], &channels[3], &channels[4], &channels[5], &channels[6], &channels[7]);
        // validation
        int enabled_channels = 0;
        for (int i = 0; i < CHANNELS; ++i)
        {
            if (!ENABLED_CHANNEL_CHECK(channels[i])) // invalid input found
            {
                invalid_input = 1;
                break;
            }
            if (channels[i])
                ++enabled_channels;
        }
        if (enabled_channels == 0) // if not one channel enabled at least
            invalid_input = 1;
    } while (invalid_input);
}

void INThandler(int _)
{
    printf("\nCatch keyboard interruption! Terminating...\n");
    close_output_fds(output_fds);
    close_arduino(arduino);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // install a keyboard INT handler
    struct sigaction act;
    act.sa_handler = INThandler;
    sigaction(SIGINT, &act, NULL);

    // welcome message
    system("clear");
    printf("Welcome to Arduino Oscilloscope\n");

    // check Arduino connection
    arduino = open_arduino();
    int res = serial_set_interface_attribs(arduino, 19200, 0);
    if (res != 0)
    {
        printf("Error during serial attributes setup\n");
        exit(EXIT_FAILURE);
    }

    // open file descriptors, store them inside an array of file descriptors
    open_output_fds(output_fds);

    while (1) // main loop
    {
        int freq, samples, mode;
        int channels[CHANNELS] = {DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED};

        char ready = 'N';

        // params setup: channels, frequency, # samples, mode
        // input param loop
        while (ready != 'Y' && ready != 'y')
        {
            // get freq param from user
            do
            {
                printf("Insert sampling frquency [1Hz - 1000Hz]: ");
                scanf("%d", &freq);
            } while (!FREQ_CHECK(freq));
            // get samples param from user
            do
            {
                printf("Insert samples [1 - 10000]: ");
                scanf("%d", &samples);
            } while (!SAMPLES_CHECK(samples));
            // get mode param from user
            do
            {
                printf("Insert mode {0: continuos, 1: buffered}: ");
                scanf("%d", &mode);
            } while (!MODE_CHECK(mode));
            // get channels param from user
            get_channels(channels);

            system("clear");
            printf("Freq: %dHz\n", freq);
            printf("Samples: %d\n", samples);
            printf("Mode: %d\n", mode);
            printf("Enabled Channels: ");
            for (int i = 0; i < CHANNELS; ++i)
                printf("%d: %d\t", i, channels[i]);
            printf("\nDo you want to start sampling? [Y/N] ");
            scanf(" %c", &ready);
            system("clear");
        }

        // format outgoing string (arduino's input)
        char out_str[DEF_BUF_SIZE];
        snprintf(out_str, DEF_BUF_SIZE, "%d %d %d,", freq, samples, mode);
        for (int i = 0; i < CHANNELS; ++i)
        {
            if (channels[i] == 1) // if channel enabled
            {
                char tmp[DEF_BUF_SIZE];
                snprintf(tmp, DEF_BUF_SIZE, " %d", i);
                strcat(out_str, tmp);
            }
        }

        // write directives to arduino
        ssize_t res = write(arduino, &out_str, strlen(out_str) + 1);
        if (res == -1)
        {
            printf("Error writing on serial\n");
            exit(EXIT_FAILURE);
        }

        char r;
        char wr_buf[8]; // len of 6 chars + \n + \0 = 8
        while (1)
        {
            res = read(arduino, &r, 1);
            // printf("%c", r);
            if (r == '-') // terminate reading cycle
                break;
            else
            {
                if (r == '\n')
                {
                    // elaboration
                    int ch;
                    float val;
                    char out[9]; // len of 7 chars + \n + \0
                    sscanf(wr_buf, "%d %f", &ch, &val);
                    val = 5 * val / 1024;
                    snprintf(out, 9, "%.5f\n", val); // string formatter + value conversion
                    // fd to be written is estabilished by ch variable which selects the correct index
                    ssize_t wr_bytes = write(output_fds[ch], out, strlen(out));
                    if (wr_bytes == -1)
                    {
                        printf("Error writing on output\n");
                        exit(EXIT_FAILURE);
                    }
                    memset(wr_buf, 0, 8); // clear wr_buf
                }
                else
                    strncat(wr_buf, &r, 1);
            }
        }

        printf("Dump completed!\n");
    }

    // close file descriptors
    close_output_fds(output_fds);
    close_arduino(arduino);

    return 0;
}