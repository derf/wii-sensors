/*
 * Copyright (C) 2011 by Daniel Friesel <derf@finalrewind.org>
 * License: WTFPL <http://sam.zoy.org/wtfpl>
 *   0. You just DO WHAT THE FUCK YOU WANT TO.
 */

/*
 * wibble: interactive wiimote blinkenlights with wobble.
 *
 * Usage: start wibble, put wiimote in discoverable mode, see blinkenlights
 *
 * Controls:
 *    D-Pad Up     toggle auto / manual mode
 *
 *    Home         quit
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <bluetooth/bluetooth.h>
#include <cwiid.h>

#define MAX_BRIGHTNESS 10
#define X_MAXP 19
#define X_MAXS 20
#define X_MAXB 36
#define X_MAX 36

cwiid_mesg_callback_t cwiid_callback;

volatile char rumble = 0;
volatile char auto_rumble = 1;

volatile char auto_mode = 1;
volatile int  cur_mode = 0;

volatile int8_t cnt_max = 7;
volatile uint8_t f_led[4][X_MAX];
volatile uint8_t x_max = X_MAXP;

struct acc_cal wm_cal;

const uint8_t stevens_io[X_MAX] = {
	0, 0, 1, 1, 1, 3, 5, 7, 9, 10, 9, 7, 5, 3, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t invb[X_MAX] = {
	0, 0, 0, 0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void set_led_fun(int new_mode)
{
	const int max_current = 5;
	int i;

	cur_mode = new_mode;
	if (cur_mode < 0)
		cur_mode = max_current;
	else if (cur_mode > max_current)
		cur_mode = 0;

	if (!auto_mode) {
		printf("\r\033[2Kmode %d", cur_mode);
		fflush(stdout);
	}

	switch (cur_mode) {
	case 0:
		x_max = X_MAXP;
		for (i = 0; i < x_max; i++) {
			f_led[0][i] = floor( 5 * ( cos( (float) (i +  0) / 3 ) + 1 ) );
			f_led[1][i] = floor( 5 * ( cos( (float) (i +  3) / 3 ) + 1 ) );
			f_led[2][i] = floor( 5 * ( cos( (float) (i +  6) / 3 ) + 1 ) );
			f_led[3][i] = floor( 5 * ( cos( (float) (i +  9) / 3 ) + 1 ) );
		}
		break;
	case 1:
		x_max = X_MAXP;
		for (i = 0; i < x_max; i++) {
			f_led[0][i] = floor( 5 * ( cos( (float) (i +  9) / 3 ) + 1 ) );
			f_led[1][i] = floor( 5 * ( cos( (float) (i +  6) / 3 ) + 1 ) );
			f_led[2][i] = floor( 5 * ( cos( (float) (i +  3) / 3 ) + 1 ) );
			f_led[3][i] = floor( 5 * ( cos( (float) (i +  0) / 3 ) + 1 ) );
		}
		break;
	case 2:
		x_max = X_MAXP;
		for (i = 0; i < x_max; i++)
			f_led[0][i] = f_led[1][i] = f_led[2][i] = f_led[3][i]
				= stevens_io[i];
		break;
	case 3:
		x_max = X_MAXB;
		for (i = 0; i < x_max; i++) {
			f_led[0][i] = f_led[1][i] = f_led[2][i] = f_led[3][i] = 0;
			if ((i%2) && (i<6))
				f_led[0][i] = MAX_BRIGHTNESS;
			if ((i%2) && (((i>=6) && (i<12)) || (i>=30)))
				f_led[1][i] = MAX_BRIGHTNESS;
			if ((i%2) && (((i>=12) && (i<18)) || ((i>=24) && (i<30))))
				f_led[2][i] = MAX_BRIGHTNESS;
			if ((i%2) && (i>=18) && (i<24))
				f_led[3][i] = MAX_BRIGHTNESS;
		}
		break;
	case 4:
		x_max = X_MAXS;
		for (i = 0; i < x_max; i++) {
			f_led[0][i] = invb[(i + 15) % x_max];
			f_led[1][i] = invb[(i + 10) % x_max];
			f_led[2][i] = invb[(i +  5) % x_max];
			f_led[3][i] = invb[i];
		}
		break;
	case 5:
		x_max = X_MAXS;
		for (i = 0; i < x_max; i++)
			f_led[0][i] = f_led[1][i] = f_led[2][i] = f_led[3][i]
				= (i % 2) * 10;
		break;
	}
}

int main()
{
	cwiid_wiimote_t *wiimote = NULL;

	struct cwiid_state state;

	uint8_t ledstate = 0x0f;

	uint8_t cnt = 0;
	uint8_t led[4] = {0, 0, 0, 0};

	uint8_t step = 0;
	uint8_t x = 0;

	uint8_t i;

	uint8_t next_mode = 0;

	puts("Press 1+2 to connect wiimote.");
	if ((wiimote = cwiid_open(BDADDR_ANY, 0)) == NULL) {
		fputs("Unable to connect\n", stderr);
		return EXIT_FAILURE;
	}

	sleep(2);

	set_led_fun(0);

	if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &wm_cal))
		fputs("unable to retrieve accelerometer calibration\n", stderr);

	if (!cwiid_get_state(wiimote, &state))
		printf("connected - battery at %d%%\n",
			(int)(100.0 * state.battery / CWIID_BATTERY_MAX));

	if (cwiid_set_mesg_callback(wiimote, cwiid_callback))
		fputs("cannot set callback. buttons won't work.\n", stderr);
	
/*	cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);
*/
	if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
		fputs("cannot enable callback. buttons won't work.\n", stderr);

	if (cwiid_set_rpt_mode(wiimote,
			CWIID_RPT_BTN | CWIID_RPT_ACC | CWIID_RPT_STATUS | CWIID_RPT_EXT))
		fputs("cannot set report mode. buttons won't work.\n", stderr);

	while (1) {

		if (++cnt >= cnt_max) {
			cnt = 0;
			
			if (++x == x_max) {
				x = 0;

				if (!auto_mode && (++next_mode == 42)) {
					set_led_fun(cur_mode + 1);
					next_mode = 0;
				}
			}

			for (i = 0; i < 4; i++)
				led[i] = f_led[i][x];
		}

		step = cnt % MAX_BRIGHTNESS;

		if (step == 0)
			ledstate = 0x0f;

		for (i = 0; i < 4; i++)
			if (step == led[i])
				ledstate &= ~(1 << i);

		if (cwiid_set_led(wiimote, ledstate))
			fputs("Error setting LED state\n", stderr);
	}

	return EXIT_SUCCESS;
}

/* 97 .. 122 .. 150 */
/* mp: normal 8200 .. 8250? */

void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	static double ff_start, ff_diff;
	static double fp_start, fp_diff;
	double a_x, a_y, a_z, accel;
	struct cwiid_acc_mesg *am;

	for (int i = 0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_BTN) {

			if (mesg[i].btn_mesg.buttons & CWIID_BTN_LEFT)
				set_led_fun(cur_mode - 1);
			if (mesg[i].btn_mesg.buttons & CWIID_BTN_RIGHT)
				set_led_fun(cur_mode + 1);
			if (mesg[i].btn_mesg.buttons & CWIID_BTN_PLUS)
				cnt_max -= (cnt_max > 1 ? 1 : 0);
			if (mesg[i].btn_mesg.buttons & CWIID_BTN_MINUS)
				cnt_max += 1;
			if (mesg[i].btn_mesg.buttons & CWIID_BTN_UP) {
				auto_mode = !auto_mode;
				if (auto_mode) {
					x_max = X_MAXP;
					puts("\r\033[2KAuto mode enabled");
					puts("- Shake to vibrate");
					puts("- Down arrow to toggle vibration");
					puts("- Tilt to change animation speed");
					puts("- Let wiimote fall to calculate fall height\n");
				} else {
					puts("Auto mode disabled");
					puts("- Down arrow to vibrate");
					puts("- Left/Right to change animation");
					puts("- +/- to change animation speed\n");
				}
			}

			if (mesg[i].btn_mesg.buttons & CWIID_BTN_DOWN) {
				if (auto_mode)
					auto_rumble = !auto_rumble;
				else if (!rumble)
					cwiid_set_rumble(wiimote, (rumble = 1));
			}
			else if (rumble && !auto_mode)
				cwiid_set_rumble(wiimote, (rumble = 0));

			if (mesg[i].btn_mesg.buttons & CWIID_BTN_HOME) {
				exit(0);
			}

		}
/*		else if ((mesg[i].type == CWIID_MESG_MOTIONPLUS) && auto_mode ) {

			if ((mesg[i].motionplus_mesg.angle_rate[2] < 8000) && !fp_start)
				fp_start = ((uint64_t)ts->tv_sec * 1000000000) + ts->tv_nsec;
			else if ((mesg[i].motionplus_mesg.angle_rate[2] > 8200) && fp_start) {
				fp_diff = ((((uint64_t)ts->tv_sec * 1000000000)
					+ ts->tv_nsec
					- ff_start) / 1000000000);

				printf("mpdel_t %.3fs - Fell approx. %.2fm\n", ff_diff,
					(double)((9.81 * (double)(fp_diff) * (double)(fp_diff)) / (double)2));
				fp_start = 0;
			}
		}
*/		else if ((mesg[i].type == CWIID_MESG_ACC) && auto_mode) {

			am = &mesg[i].acc_mesg;

			a_x = ((double)am->acc[CWIID_X] - wm_cal.zero[CWIID_X]) /
				(wm_cal.one[CWIID_X] - wm_cal.zero[CWIID_X]);
			a_y = ((double)am->acc[CWIID_Y] - wm_cal.zero[CWIID_Y]) /
				(wm_cal.one[CWIID_Y] - wm_cal.zero[CWIID_Y]);
			a_z = ((double)am->acc[CWIID_Z] - wm_cal.zero[CWIID_Z]) /
				(wm_cal.one[CWIID_Z] - wm_cal.zero[CWIID_Z]);
			accel = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));

			if ((accel < 0.07) && !ff_start)
				ff_start = ((uint64_t)ts->tv_sec * 1000000000) + ts->tv_nsec;
			else if ((accel > 1.0) && ff_start) {
				ff_diff = ((((uint64_t)ts->tv_sec * 1000000000)
					+ ts->tv_nsec
					- ff_start) / 1000000000);

				printf("delta_t %.3fs - Fell approx. %.2fm\n", ff_diff,
					(double)((9.81 * (double)(ff_diff) * (double)(ff_diff)) / (double)2));
				ff_start = 0;
			}

			if (auto_rumble && (accel > 1.5)) {
				if (!rumble)
					cwiid_set_rumble(wiimote, (rumble = 1));
			}
			else {
				if (rumble)
					cwiid_set_rumble(wiimote, (rumble = 0));
			}

			if (mesg[i].acc_mesg.acc[CWIID_X] < 123) {

				if (cur_mode != 0)
					set_led_fun(0);

				cnt_max = (mesg[i].acc_mesg.acc[CWIID_X] - 95) / 2;
				if (cnt_max < 2)
					cnt_max = 2;
			}
			else {

				if (cur_mode != 1)
					set_led_fun(1);

				cnt_max = (150 - mesg[i].acc_mesg.acc[CWIID_X]) / 2;
				if (cnt_max < 2)
					cnt_max = 2;
			}
		}
	}
}

