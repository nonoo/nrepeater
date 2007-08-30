#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <parapindriver.h>

#define CLK LP_PIN01
#define DATA LP_PIN11
#define NCS LP_PIN02


int main(int argc, char *argv[])
{
  int i, value;
  int device;
  float temp;

  /* Get things set up */
  device = open("/dev/ppdrv_device", 0);

  if (device < 0) {
    fprintf(stderr, "device open failed, we're outta here\n");
    exit(-1);
  }

  ioctl(device, PPDRV_IOC_PINMODE_OUT, CLK | NCS);
  ioctl(device, PPDRV_IOC_PINMODE_IN, DATA);

  /* make sure we're in the idle state */
  ioctl(device, PPDRV_IOC_PINSET, NCS);
  ioctl(device, PPDRV_IOC_PINCLEAR, CLK);

  /* start a conversion by dropping NCS */
  ioctl(device, PPDRV_IOC_PINCLEAR, NCS);

  /* wait for data to go high, signalling the end of conversion */
  while (!ioctl(device, PPDRV_IOC_PINGET, DATA))
    /* null */;

  /* clock out 12 bits of data */
  value = 0;
  for (i=0; i<12; i++) {
    value <<= 1;
    ioctl(device, PPDRV_IOC_PINSET, CLK);
    ioctl(device, PPDRV_IOC_PINCLEAR, CLK);
    value |= (ioctl(device, PPDRV_IOC_PINGET, DATA) ? 1 : 0);
  }

   /* go back to idle */
  ioctl(device, PPDRV_IOC_PINCLEAR, CLK);
  ioctl(device, PPDRV_IOC_PINSET, NCS);

  /* print out the values */
  temp = ((float)value / (float)0x0FFF) * 1250;
  printf("value(hex)=%x;  degC=%.0f;  degF=%.0f\n", value, temp, (temp*(9.0/5.0))+32);


  /* close the device */
  close(device);

  return 0;
}
