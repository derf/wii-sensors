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

int main()
{
	cwiid_wiimote_t *wiimote = NULL;

	if ((wiimote = cwiid_open(BDADDR_ANY, 0)) == NULL) {
		fputs("Unable to connect\n", stderr);
		return EXIT_FAILURE;
	}

	sleep(2);

	cwiid_set_led(wiimote, (1 << 3) | (1));

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

void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	for (int i = 0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_MOTIONPLUS) {
			printf("%ld.%09ld %d %d %d\n", ts->tv_sec, ts->tv_nsec,
				mesg[i].motionplus_mesg.angle_rate[0],
				mesg[i].motionplus_mesg.angle_rate[1],
				mesg[i].motionplus_mesg.angle_rate[2]
			);
		}
	}
}

