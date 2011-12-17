/*
 * Copyright (C) 2011 by Daniel Friesel <derf@finalrewind.org>
 * License: WTFPL <http://sam.zoy.org/wtfpl>
 *   0. You just DO WHAT THE FUCK YOU WANT TO.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <bluetooth/bluetooth.h>
#include <cwiid.h>

#define NUM_VALUES 1000

cwiid_mesg_callback_t cwiid_callback;

volatile unsigned short values[3][NUM_VALUES];

int main()
{
	cwiid_wiimote_t *wiimote = NULL;

	fputs("Press 1+2 to connect wiimote\n", stdout);
	fputs("Put it onto a flat surface and do not move it\n", stdout);

	if ((wiimote = cwiid_open(BDADDR_ANY, 0)) == NULL) {
		fputs("Unable to connect\n", stderr);
		return EXIT_FAILURE;
	}

	sleep(2);

	cwiid_set_led(wiimote, 1);

	if (cwiid_set_mesg_callback(wiimote, cwiid_callback))
		fputs("cannot set callback\n", stderr);
	
	cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);

	if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
		fputs("cannot enable callback\n", stderr);

	if (cwiid_set_rpt_mode(wiimote,
			CWIID_RPT_EXT))
		fputs("cannot set report mode\n", stderr);

	while (1) {
		/* nothing to do here */
		sleep(1);
	}

	return EXIT_SUCCESS;
}

void show_calibration()
{
	long unsigned int total[3] = {0, 0, 0};
	unsigned short min[3], max[3];

	min[0] = max[0] = values[0][0];
	min[1] = max[1] = values[1][0];
	min[2] = max[2] = values[2][0];

	for (int step = 0; step < NUM_VALUES; step++) {
		for (int i = 0; i < 3; i++) {

			total[i] += values[i][step];

			if (values[i][step] < min[i])
				min[i] = values[i][step];

			if (values[i][step] > max[i])
				max[i] = values[i][step];
		}
	}

	printf("\r\033[2K        avg  ( min  ..  max )\n");
	printf("Pitch: %5lu (%5d .. %5d)\n",
		total[0] / NUM_VALUES, min[0], max[0]);
	printf("Roll : %5lu (%5d .. %5d)\n",
		total[1] / NUM_VALUES, min[1], max[1]);
	printf("Yaw  : %5lu (%5d .. %5d)\n",
		total[2] / NUM_VALUES, min[2], max[2]);

	exit(EXIT_SUCCESS);
}


void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	static int cnt = 0;
	static int skip = 100;

	if (skip > 0) {
		skip--;
		return;
	}

	if (!(cnt % 100)) {
		printf("\r\033[2K%4d / %d", cnt, NUM_VALUES);
		fflush(stdout);
	}

	if (cnt == (NUM_VALUES/4))
		cwiid_set_led(wiimote, 0x3);

	else if (cnt == (NUM_VALUES/2))
		cwiid_set_led(wiimote, 0x7);

	else if (cnt == (NUM_VALUES*3/4))
		cwiid_set_led(wiimote, 0xf);

	for (int i = 0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_MOTIONPLUS) {
			values[0][cnt] = mesg[i].motionplus_mesg.angle_rate[0];
			values[1][cnt] = mesg[i].motionplus_mesg.angle_rate[1];
			values[2][cnt] = mesg[i].motionplus_mesg.angle_rate[2];
			cnt++;

			if (cnt >= NUM_VALUES)
				show_calibration();
		}
	}
}

