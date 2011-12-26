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

cwiid_mesg_callback_t cwiid_callback;

struct acc_cal wm_cal;

int main()
{
	cwiid_wiimote_t *wiimote = NULL;

	if ((wiimote = cwiid_open(BDADDR_ANY, 0)) == NULL) {
		fputs("Unable to connect\n", stderr);
		return EXIT_FAILURE;
	}

	sleep(2);

	cwiid_set_led(wiimote, (1 << 3) | (1));

	if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &wm_cal))
		fputs("unable to retrieve accelerometer calibration\n", stderr);

	if (cwiid_set_mesg_callback(wiimote, cwiid_callback))
		fputs("cannot set callback\n", stderr);
	
	if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
		fputs("cannot enable callback\n", stderr);

	if (cwiid_set_rpt_mode(wiimote,
			CWIID_RPT_ACC))
		fputs("cannot set report mode\n", stderr);

	while (1) {
		/* nothing to do here */
		sleep(1);
	}

	return EXIT_SUCCESS;
}

void handle_acc(cwiid_wiimote_t *wiimote, struct cwiid_acc_mesg *am,
	struct timespec *ts)
{
	double a_x = ((double)am->acc[CWIID_X] - wm_cal.zero[CWIID_X]) /
		(wm_cal.one[CWIID_X] - wm_cal.zero[CWIID_X]);
	double a_y = ((double)am->acc[CWIID_Y] - wm_cal.zero[CWIID_Y]) /
		(wm_cal.one[CWIID_Y] - wm_cal.zero[CWIID_Y]);
	double a_z = ((double)am->acc[CWIID_Z] - wm_cal.zero[CWIID_Z]) /
		(wm_cal.one[CWIID_Z] - wm_cal.zero[CWIID_Z]);
	double accel = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
	double roll = atan(a_x / a_z);
	if (a_z <= 0.0) {
		roll += 3.14159265358979323 * ((a_x > 0.0) ? 1 : -1);
	}
	double pitch = atan( a_y / a_z * cos(roll));

	printf("%ld.%09ld %f %f %f %f %f %f\n",
		ts->tv_sec, ts->tv_nsec,
		a_x, a_y, a_z,
		accel, roll, pitch
	);
}

void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	for (int i = 0; i < mesg_count; i++)
		if (mesg[i].type == CWIID_MESG_ACC)
			handle_acc(wiimote, &mesg[i].acc_mesg, ts);
}

