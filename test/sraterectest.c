#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <samplerate.h>

int fd_in;
FILE* fd_out;
int sample_rate = 44100;
SRC_STATE* sstate = NULL;
SRC_DATA sdata;
float srcin[1024];
float srcout[1024];
short dataout[1024];

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

    sdata.input_frames = l;
    src_short_to_float_array( buffer, srcin, l );
    src_process( sstate, &sdata );
    src_float_to_short_array( srcout, dataout, sdata.output_frames_gen );
    fwrite( dataout, 1, sdata.output_frames_gen*sizeof(short), fd_out );

    level = 0;

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

    int tmp=0;
    sstate = src_new( SRC_SINC_BEST_QUALITY, 1, &tmp );
    sdata.output_frames = 1024;
    sdata.data_in = srcin;
    sdata.data_out = srcout;
    sdata.end_of_input = 0;
    sdata.src_ratio = (1.0*8000) / sample_rate;

    while (1)
	process_input ();

    sstate = src_delete( sstate );
    fclose( fd_out );

    exit (0);
}
