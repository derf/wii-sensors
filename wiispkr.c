/*
 * Copyright (C) 2011 by Daniel Friesel <derf@finalrewind.org>
 * License: WTFPL:
 *   0. You just DO WHAT THE FUCK YOU WANT TO
 *
 * Loosely based on libcwiimote/test/test2.c
 *
 *
 * A note on sample rates:
 * Right now we use 2kHz with 8bit signed pcm.
 * The speaker eats up to 20 bytes of raw data at once, so this gives us
 * 100 updates per second (one every 10 us).
 * Higher rates like 3kHz don't work as well, looks like one report
 * every 10us is about as fast as the bluetooth chipset / driver here gets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#include <libcwiimote/wiimote.h>
#include <libcwiimote/wiimote_api.h>

#define SPK_REG_CFG		0x04a20001
#define SPK_REG_VOL		0x04a20005
#define SPK_REG_FREQ	0x04a20004

void print_sample(uint8_t *sample, int length)
{
	for (int i = 0; i < length; i++)
		printf("%02x, ", sample[i]);
	puts("");
}

int main(int argc, char **argv)
{
	wiimote_t wiimote = WIIMOTE_INIT;
	int readbytes;
	char report_err = 0;

	wiimote_report_t report = WIIMOTE_REPORT_INIT;
	report.channel = WIIMOTE_RID_SPK;

	struct timeval tv;
	uint32_t timeout = 0;
	uint32_t t, start;

	uint32_t fpos = 0;
	
	uint8_t sample[20];

	/* see http://wiibrew.org/wiki/Wiimote#Speaker_Configuration */
	uint8_t spk_8bit[] = { 0x00, 0x40, 0x70, 0x17, 0x60, 0x00, 0x00 };


	if (argc < 2) {
		fprintf(stderr, "Usage: %s BDADDR < somefile.raw\n", argv[0]);
		return 1;
	}
	
	puts("Press 1+2 to connect wiimote");
	
	if (wiimote_connect(&wiimote, argv[1]) < 0) {
		fprintf(stderr, "unable to open wiimote: %s\n", wiimote_get_error());
		exit(1);
	}

	wiimote.led.one  = 1;
	
	wiimote_update(&wiimote);

	wiimote_send_byte(&wiimote, WIIMOTE_RID_SPK_EN, 0x04);
	wiimote_send_byte(&wiimote, WIIMOTE_RID_SPK_MUTE, 0x04);
	wiimote_write_byte(&wiimote, 0x04a20009, 0x01);
	wiimote_write_byte(&wiimote, 0x04a20001, 0x08);

	wiimote_write(&wiimote, 0x04a20001, spk_8bit, sizeof(spk_8bit));

	wiimote_write_byte(&wiimote, 0x04a20008, 0x01);
	wiimote_send_byte(&wiimote, WIIMOTE_RID_SPK_MUTE, 0x00);

	gettimeofday(&tv, NULL);
	start = (tv.tv_sec * 1000000 + tv.tv_usec) / 100;

	while (wiimote_is_open(&wiimote)) {
		gettimeofday(&tv,NULL);
		t = (tv.tv_sec * 1000000 + tv.tv_usec) / 100;

		if (t > timeout) {
			printf("\r\033[2K%c play second %f pos %d",
				(report_err ? '!' : ' '),
				(double)(t - start) / 10000, fpos);
			fflush(stdout);
			if ((readbytes = read(0, &sample, sizeof(sample))) < 1)
				return 0;
			fpos += readbytes;

			report.speaker.size = sizeof(sample);
			memcpy(report.speaker.data, sample, report.speaker.size);
			if (wiimote_report(&wiimote, &report, sizeof(report.speaker)) < 0)
				report_err = 1;
			else
				report_err = 0;

			timeout = t + 100;
		}
		else
			usleep((timeout - t) * 10);
	}
	
	return 0;
}
