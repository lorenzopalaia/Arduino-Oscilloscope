#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define CHANNELS 8

#define DEFAULT_BUFFERS_SIZE 256

// validation macros
#define FREQ_CHECK(FREQ) (FREQ >= 1 && FREQ <= 10000)
#define SAMPLES_CHECK(SAMPLES) (SAMPLES >= 1 && SAMPLES <= 10000)
#define MODE_CHECK(MODE) (MODE == 0 || MODE == 1)
#define ENABLED_CHANNEL_CHECK(ENABLED) (ENABLED == 0 || ENABLED == 1)

void open_output_fds(int output_fds[CHANNELS])
{
    for (int i = 0; i < CHANNELS; ++i)
    {
        char out_path[24];
        snprintf(out_path, 24, "../outputs/out_ch%d.txt", i);
        int fd = open(out_path, O_WRONLY);
        if (fd < 0) // check opening errors
        {
            printf("Error opening output files! Missing: %s\n", out_path);
            exit(EXIT_FAILURE);
        }
        output_fds[i] = fd;
    }
}

int open_arduino()
{
    FILE *fp = popen("find /dev/cu.usbmodem*", "r"); // subprocess
    char path[DEFAULT_BUFFERS_SIZE];
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
        printf("Error opening!\n");
        exit(EXIT_FAILURE);
    }
    printf("Arduino connection open!\n");
    return fd;
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
        char input[DEFAULT_BUFFERS_SIZE];
        printf("Insert enabled channels (with spacing) [8 values: {0: disabled, 1: enabled}]: ");
        scanf("%d %d %d %d %d %d %d %d", &channels[0], &channels[1], &channels[2], &channels[3], &channels[4], &channels[5], &channels[6], &channels[7]);
        // validation
        int enabled = 0;
        for (int i = 0; i < CHANNELS; ++i)
        {
            if (!ENABLED_CHANNEL_CHECK(channels[i])) // invalid input found
            {
                invalid_input = 1;
                break;
            }
            if (channels[i])
                ++enabled;
        }
        if (enabled == 0) // if not one channel enabled at least
            invalid_input = 1;
    } while (invalid_input);
}

int main(int argc, char *argv[])
{
    // controllare connessione con arduino
    // TODO: imposta parametri di connessione con arduino
    // loop infinito
    //   DONE: imposta parametri: canali, frequenza, # campioni, modalità
    //   DONE: mostra resoconto e chiedi conferma
    //   DONE: invia parametri ad Arduino
    //   DONE: ricevi samples
    //   TODO: dumpa su file
    //   TODO: controllare chiusura di tutto

    // open file descriptors, store them inside an array of file descriptors
    int output_fds[CHANNELS];
    open_output_fds(output_fds);

    // welcome message
    system("clear");
    printf("Welcome to Arduino Oscilloscope\n");

    // check Arduino connection
    printf("Looking for Arduino on serial port...\n");
    int arduino = open_arduino();
    int res = serial_set_interface_attribs(arduino, 19200, 0);
    if (res != 0)
    {
        printf("Error during serial attributes setup\n");
        exit(EXIT_FAILURE);
    }

    while (1) // main loop
    {
        int freq;
        int samples;
        int mode;
        int channels[CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0}; // 0: disabled, 1: enabled

        char ready = 'N';

        // params setup: channels, frequency, # samples, mode
        while (ready != 'Y' && ready != 'y')
        {
            // input param loop

            // get freq param from user
            do
            {
                printf("Insert sampling frquency [1Hz - 10000Hz]: ");
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
            printf("\nSampled output length will be %.2fs\n", 1 / (float)freq * samples);
            printf("Do you want to start sampling? [Y/N] ");
            scanf(" %c", &ready);
            system("clear");
        }

        // format out string
        char out_str[DEFAULT_BUFFERS_SIZE];
        snprintf(out_str, DEFAULT_BUFFERS_SIZE, "%d %d %d,", freq, samples, mode);
        for (int i = 0; i < CHANNELS; ++i)
        {
            if (channels[i] == 1) // if channel enabled
            {
                char tmp[DEFAULT_BUFFERS_SIZE];
                snprintf(tmp, DEFAULT_BUFFERS_SIZE, " %d", i);
                strcat(out_str, tmp);
            }
        }

        // printf("Sending: %s\n", out_str);
        ssize_t res = write(arduino, &out_str, sizeof(out_str));
        if (res == -1)
        {
            printf("Error writing on serial\n");
            exit(EXIT_FAILURE);
        }

        // calculate size of recv buffer
        // required samples * channels (= 8, fixed, worst case scenario) * recv string format length (worst case scenario)
        int RECV_BUF_SIZE = samples * CHANNELS * (sizeof(samples) + sizeof(" ") + sizeof(CHANNELS) + sizeof(" ") + sizeof(int));
        int test_size = 6 * sizeof(char) + sizeof("\n");
        char recv_buf[test_size];
        // reading cycle
        // TODO: improve everything
        //      read line by line
        //          then append to recv buffer
        //          or
        //          handle line: tokenize, append to respective dumping file (this way should minimize recv buffer size, just recv string length)
        //      stop reading (read return value (best way)? stopping msg by arduino (worst way)?)

        while (1)
        {
            // set res to read bytes, then add string terminator
            res = read(arduino, recv_buf, sizeof(recv_buf) - 1);
            if (res < 0)
                break;
            else
            {
                recv_buf[res] = '\0';
                int ch, val;
                printf("%s", recv_buf);
                // sscanf(recv_buf, "%d %d", &ch, &val);
                // printf("%d %d", ch, val);
            }
        }
    }

    return 0;
}