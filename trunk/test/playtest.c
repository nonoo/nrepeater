#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "soundcard.h"

int fd_out;
int sample_rate = 44100;

static void write_sinewave (void)
{
    static unsigned int phase = 0;	/* Phase of the sine wave */
    unsigned int p;
    int i;
    short buf[1024];		/* 1024 samples/write is a safe choice */

    int outsz = sizeof (buf) / 2;

    static int sinebuf[48] = {
	        0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
		28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
		28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
		0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
		-28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
		-28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
    };

    for (i = 0; i < outsz; i++)
    {
        p = (phase * sample_rate) / 44100;

	phase = (phase + 1) % 4800;
	buf[i] = sinebuf[p % 48];
    }

    if (write (fd_out, buf, sizeof (buf)) != sizeof (buf))
    {
	perror ("Audio write");
	exit (-1);
    }
}

static int open_audio_device (char *name, int mode)
{
    int tmp, fd;

    if ((fd = open (name, mode, 0)) == -1)
    {
	perror (name);
	exit (-1);
    }

    tmp = AFMT_S16_NE;		/* Native 16 bits */
    if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
	perror ("SNDCTL_DSP_SETFMT");
	exit (-1);
    }

    if (tmp != AFMT_S16_NE)
    {
	fprintf (stderr, "The device doesn't support the 16 bit sample format.\n");
	exit (-1);
    }

    tmp = 1;
    if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
	perror ("SNDCTL_DSP_CHANNELS");
	exit (-1);
    }

    if (tmp != 1)
    {
	fprintf (stderr, "The device doesn't support mono mode.\n");
	exit (-1);
    }

    sample_rate = 44100;
    if (ioctl (fd, SNDCTL_DSP_SPEED, &sample_rate) == -1)
    {
	perror ("SNDCTL_DSP_SPEED");
	exit (-1);
    }

    return fd;
}

int main (int argc, char *argv[])
{
    char *name_out = "/dev/dsp";

    if (argc > 1)
        name_out = argv[1];

       fd_out = open_audio_device (name_out, O_WRONLY);

	while (1)
	    write_sinewave ();

    exit (0);
}
