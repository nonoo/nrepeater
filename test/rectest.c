#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>

int fd_in;
FILE* fd_out;
int sample_rate = 44100;

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
	  
void process_input (void)
{
    short buffer[1024];

    int l, i, level;

    if ((l = read (fd_in, buffer, sizeof (buffer))) == -1)
    {
	perror ("Audio read");
	exit (-1);		/* Or return an error code */
    }
    l = l / 2;
    level = 0;

    fwrite( &buffer, 2048, 1, fd_out );

    for (i = 0; i < l; i++)
    {
	int v = buffer[i];

	if (v < 0)
	v = -v;			/* abs */

	if (v > level)
	level = v;
    }
    level = (level + 1) / 1024;

    for (i = 0; i < level; i++)
	printf ("*");
    for (i = level; i < 32; i++)
	printf (".");
    printf ("\r");
    fflush (stdout);
}
		  
int main (int argc, char *argv[])
{
    char *name_in = "/dev/dsp";

    if (argc > 1)
	name_in = argv[1];
    fd_in = open_audio_device (name_in, O_RDONLY);

    fd_out = fopen( "out.raw", "w" );

    while (1)
	process_input ();

    fclose( fd_out );

    exit (0);
}
