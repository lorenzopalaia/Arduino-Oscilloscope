#include <stdio.h>
#include <stdlib.h>

#define CHANNELS 8

#define FREQ_CHECK(FREQ) (FREQ >= 1 && FREQ <= 10000)
#define SAMPLES_CHECK(SAMPLES) (SAMPLES >= 1 && SAMPLES <= 10000)
#define MODE_CHECK(MODE) (MODE == 0 || MODE == 1)

int main(int argc, char *argv[])
{
    // TODO: controllare connessione con arduino
    // loop infinito
    //   DONE: imposta parametri: canali, frequenza, # campioni, modalità
    //   DONE: mostra resoconto e chiedi conferma
    //   TODO: invia parametri ad Arduino
    //   TODO: attesa elaborazione
    //   TODO: ricevi samples
    //   TODO: dumpa su file

    system("clear");
    // welcome message

    // check Arduino connection

    while (1) // main loop
    {
        int freq;
        int samples;
        int mode;
        int channels[CHANNELS];

        char ready = 'N';

        // params setup: channels, frequency, # samples, mode
        while (ready != 'Y' && ready != 'y')
        {
            system("clear");
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
            // do
            // {
            //     printf("Insert channels [0 - 7]: ");
            //     // scanf("%s", input);
            //     printf("%s", input);
            // } while (!MODE_CHECK(mode));

            system("clear");

            printf("Freq: %dHz\n", freq);
            printf("Samples: %d\n", samples);
            printf("Mode: %d\n", mode);

            printf("Sampled output length will be %.2fs\n", 1 / (float)freq * samples);

            printf("Do you want to start sampling? [Y/N] ");
            scanf(" %c", &ready);
        }

        // Send params to Arduino
        printf("Sent");
    }

    return 0;
}