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
#include <math.h>

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

double getvolume(uint8_t *sample, int length)
{
	uint64_t sum;
	double rms;
	static double minrms = 0, maxrms = 0;
	static int skip = 0;

	if (skip != 10) {
		skip++;
		return 0.0;
	}

	for (int i = 0; i < length; i++)
		sum += (int8_t)sample[i] * (int8_t)sample[i];
	rms = sqrt(sum);

	if ((rms < minrms) || (minrms == 0))
		minrms = rms;
	if ((rms > maxrms))
		maxrms = (rms + maxrms) / 2;

	return (rms - minrms) / (maxrms - minrms) * 5.0;
}

int main(int argc, char **argv)
{
	wiimote_t wiimote = WIIMOTE_INIT;
	int readbytes;
	char report_err = 0;

	wiimote_report_t report = WIIMOTE_REPORT_INIT;
	wiimote_report_t reportl = WIIMOTE_REPORT_INIT;

	struct timeval tv;
	uint32_t timeout = 0;
	uint32_t t, start;

	uint32_t fpos = 0;
	uint8_t ledbits = 0;
	uint8_t cnt = 0;
	uint8_t wmvolume = 0x40;
	
	uint8_t sample[20];
	double volume;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s BDADDR < somefile.raw\n", argv[0]);
		return 1;
	}
	if (argc >= 3)
		wmvolume = atoi(argv[2]);

	/* see http://wiibrew.org/wiki/Wiimote#Speaker_Configuration */
	uint8_t spk_8bit[] = { 0x00, 0x40, 0x70, 0x17, wmvolume, 0x00, 0x00 };
	
	puts("Press 1+2 to connect wiimote");
	
	if (wiimote_connect(&wiimote, argv[1]) < 0) {
		fprintf(stderr, "unable to open wiimote: %s\n", wiimote_get_error());
		exit(1);
	}

	wiimote.led.one  = 1;
	
	wiimote_update(&wiimote);

	sleep(2);

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
			if ((readbytes = read(0, &sample, sizeof(sample))) < 1)
				return 0;
			fpos += readbytes;

			volume = getvolume(sample, sizeof(sample));

			switch ((uint8_t)volume) {
				case 0: ledbits = 0x0; break;
				case 1: ledbits = 0x1; break;
				case 2: ledbits = 0x3; break;
				case 3: ledbits = 0x7; break;
				case 4:
				case 5: ledbits = 0xf; break;
			}

			printf("\r\033[2Kplaying %7.3f @ %6d \033[1m%c\033[0m  ",
				(double)(t - start) / 10000,
				fpos,
				(report_err ? '!' : ' ')
			);
			fflush(stdout);

			if (cnt++ == 2) {
				reportl.channel = WIIMOTE_RID_LEDS;
				reportl.led.leds = ledbits;
				reportl.led.rumble = 0;

				wiimote_report(&wiimote, &reportl, sizeof(reportl.led));
				cnt = 0;
			}

			report.channel = WIIMOTE_RID_SPK;
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
