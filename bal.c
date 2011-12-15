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


struct balance_cal balance_cal;

/* reported calibration:
 * left  top 16321 18090 19865
 * right top  6642  8409 10177
 * left  bot  3113  4888  6668
 * right bot  8494 10223 11951
 */
double weight(uint16_t reading, uint16_t cal[3])
{
/*	if (reading < cal[0])
		return 0.0;
*/	if (reading < cal[1])
		return ((double)reading - cal[0]) / (cal[1] - cal[0]) * 17.0;
	else
		return (((double)reading - cal[1]) / (cal[2] - cal[1]) * 17.0) + 17.0;
}

int main()
{
	cwiid_wiimote_t *wiimote = NULL;

	struct cwiid_state state;

	double wlt, wrt, wlb, wrb;
	double bal_x, bal_y;

	if ((wiimote = cwiid_open(BDADDR_ANY, 0)) == NULL) {
		fputs("Unable to connect\n", stderr);
		return EXIT_FAILURE;
	}
	fputs("connected\n", stdout);

	sleep(2);

	if (cwiid_set_led(wiimote, 1))
		fputs("Unable to set LED state\n", stderr);

	if (cwiid_get_balance_cal(wiimote, &balance_cal))
		fputs("unable to retrieve balance calibration\n", stderr);

	printf("bcal %d/%d/%d %d/%d/%d\n     %d/%d/%d %d/%d/%d\n",
		balance_cal.left_top[0],
		balance_cal.left_top[1],
		balance_cal.left_top[2],
		balance_cal.right_top[0],
		balance_cal.right_top[1],
		balance_cal.right_top[2],
		balance_cal.left_bottom[0],
		balance_cal.left_bottom[1],
		balance_cal.left_bottom[2],
		balance_cal.right_bottom[0],
		balance_cal.right_bottom[1],
		balance_cal.right_bottom[2]
	);

	if (!cwiid_get_state(wiimote, &state))
		printf("battery at %d%%\n",
			(int)(100.0 * state.battery / CWIID_BATTERY_MAX));

	if (cwiid_set_mesg_callback(wiimote, cwiid_callback))
		fputs("cannot set callback. buttons won't work.\n", stderr);
	
	if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
		fputs("cannot enable callback. buttons won't work.\n", stderr);

	if (cwiid_set_rpt_mode(wiimote,
			CWIID_RPT_ACC | CWIID_RPT_STATUS | CWIID_RPT_EXT))
		fputs("cannot set report mode. buttons won't work.\n", stderr);

	while (1) {
		cwiid_get_state(wiimote, &state);

		wlt = weight(state.ext.balance.left_top, balance_cal.left_top);
		wrt = weight(state.ext.balance.right_top, balance_cal.right_top);
		wlb = weight(state.ext.balance.left_bottom, balance_cal.left_bottom);
		wrb = weight(state.ext.balance.right_bottom, balance_cal.right_bottom);

		bal_x = (wrt + wrb) / (wlt + wlb);
		if (bal_x > 1)
			bal_x = ((wlt + wlb) / (wrt + wrb) * (-1.0)) + 1.0;
		else
			bal_x -= 1;

		bal_y = (wlt + wrt) / (wlb + wrb);
		if (bal_y > 1)
			bal_y = ((wlb + wrb) / (wlt + wrt) * (-1.0)) + 1.0;
		else
			bal_y -= 1;

		printf("%6.1f kg  %6.1f kg    %04x %04x     (%5.1f kg)\n%6.1f kg  %6.1f kg    %04x %04x\n\n",
			wlt, wrt,
			state.ext.balance.left_top, state.ext.balance.right_top,
			wlt + wrt + wlb + wrb,
			wlb, wrb,
			state.ext.balance.left_bottom, state.ext.balance.right_bottom
		);
		printf("balance %6f %6f\n\n", bal_x, bal_y);
		sleep(1);
	}

	return EXIT_SUCCESS;
}

/* 97 .. 122 .. 150 */
/* mp: normal 8200 .. 8250? */

void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	for (int i = 0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_BALANCE) {
		}

	}
}

