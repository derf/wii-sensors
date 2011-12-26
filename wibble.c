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

volatile int  cur_mode = 0;

volatile enum {
	AUTO_MODE, BLINKEN_MODE, DRAW_MODE, DRAWCIRCLE_MODE, INVAL_MODE
} opmode = AUTO_MODE;

volatile int8_t cnt_max = 7;
volatile uint8_t f_led[4][X_MAX];
volatile uint8_t x_max = X_MAXP;

volatile char drawing = 0;

struct acc_cal wm_cal;

const uint8_t stevens_io[X_MAX] = {
	0, 0, 1, 1, 1, 3, 5, 7, 9, 10, 9, 7, 5, 3, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t invb[X_MAX] = {
	0, 0, 0, 0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void set_next_mode()
{
	opmode++;
	if (opmode == INVAL_MODE)
		opmode = AUTO_MODE;
}

void set_led_fun(int new_mode)
{
	const int max_current = 6;
	int i;

	cur_mode = new_mode;
	if (cur_mode < 0)
		cur_mode = max_current;
	else if (cur_mode > max_current)
		cur_mode = 0;

	if (opmode == BLINKEN_MODE) {
		printf("\r\033[2Kanimation %d", cur_mode);
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
	case 6:
		x_max = X_MAXS;
		for (i = 0; i < x_max; i++) {
			f_led[0][i] = ((i % 10) == 0) * 10;
			f_led[1][i] = ((i % 10) == 1) * 10;
			f_led[2][i] = ((i % 10) == 2) * 10;
			f_led[3][i] = ((i % 10) == 3) * 10;
		}
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
	
	if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
		fputs("cannot enable callback. buttons won't work.\n", stderr);

	if (cwiid_set_rpt_mode(wiimote,
			CWIID_RPT_BTN | CWIID_RPT_ACC | CWIID_RPT_STATUS | CWIID_RPT_EXT))
		fputs("cannot set report mode. buttons won't work.\n", stderr);

	while (1) {

		if (opmode == DRAWCIRCLE_MODE) {
			sleep(1);
			continue;
		}

		if (++cnt >= cnt_max) {
			cnt = 0;
			
			if (++x == x_max) {
				x = 0;

				if ((opmode == BLINKEN_MODE) && (++next_mode == 42)) {
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

void handle_button_normal(cwiid_wiimote_t *wiimote,
	uint16_t buttons)
{
	if (buttons & CWIID_BTN_LEFT)
		set_led_fun(cur_mode - 1);

	if (buttons & CWIID_BTN_RIGHT)
		set_led_fun(cur_mode + 1);

	if (buttons & CWIID_BTN_PLUS)
		cnt_max -= (cnt_max > 1 ? 1 : 0);

	if (buttons & CWIID_BTN_MINUS)
		cnt_max += 1;

	if (buttons & CWIID_BTN_DOWN) {
		if (opmode == AUTO_MODE)
			auto_rumble = !auto_rumble;
		else if (!rumble && (opmode == BLINKEN_MODE))
			cwiid_set_rumble(wiimote, (rumble = 1));
	}
	else if (rumble && (opmode == BLINKEN_MODE))
		cwiid_set_rumble(wiimote, (rumble = 0));
}

void handle_button_draw(cwiid_wiimote_t *wiimote,
	uint16_t buttons)
{
	if (buttons & (CWIID_BTN_A | CWIID_BTN_1))
		drawing = 1;
	else if (buttons & (CWIID_BTN_B | CWIID_BTN_2))
		drawing = -1;
	else
		drawing = 0;
}

/* 97 .. 122 .. 150 */
/* mp: normal 8200 .. 8250? */

void handle_acc_auto(cwiid_wiimote_t *wiimote, struct cwiid_acc_mesg *am,
	struct timespec *ts)
{
	static double ff_start = 0.0;
	static double w_start = 0.0, w_last = 0.0;
	static int w_count = 0;
	double ff_diff;

	double a_x = ((double)am->acc[CWIID_X] - wm_cal.zero[CWIID_X]) /
		(wm_cal.one[CWIID_X] - wm_cal.zero[CWIID_X]);
	double a_y = ((double)am->acc[CWIID_Y] - wm_cal.zero[CWIID_Y]) /
		(wm_cal.one[CWIID_Y] - wm_cal.zero[CWIID_Y]);
	double a_z = ((double)am->acc[CWIID_Z] - wm_cal.zero[CWIID_Z]) /
		(wm_cal.one[CWIID_Z] - wm_cal.zero[CWIID_Z]);
	double accel = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));

	double now = ((double)ts->tv_nsec / 1000000000) + (double)ts->tv_sec;

	if ((accel < 0.07) && !ff_start)
		ff_start = now;
	else if ((accel > 1.0) && ff_start) {
		ff_diff = now - ff_start;

		printf("delta_t %.3fs - Fell approx. %.2fm\n", ff_diff,
			(double)((9.81 * (double)(ff_diff) * (double)(ff_diff)) / (double)2));
		ff_start = 0;
	}

	if ((a_y < -2) && !w_start)
		w_start = now;
	else if (a_y < -2) {
		w_count++;
		w_last = now;
		if (w_count > 5) {
			printf("\r\033[2K8====) Wanking for %.f seconds at %4.1fHz",
				w_last - w_start,
				w_count / (w_last - w_start)
			);
			fflush(stdout);
		}
	}

	if (w_last && ((now - w_last) > 5)) {
		w_count = 0;
		w_start = w_last = 0.0;
		fputs("\r\033[2K", stdout);
		fflush(stdout);
	}

	if (auto_rumble && (accel > 1.5)) {
		if (!rumble)
			cwiid_set_rumble(wiimote, (rumble = 1));
	}
	else {
		if (rumble)
			cwiid_set_rumble(wiimote, (rumble = 0));
	}

	if (am->acc[CWIID_X] < 123) {

		if (cur_mode != 0)
			set_led_fun(0);

		cnt_max = (am->acc[CWIID_X] - 95) / 2;
		if (cnt_max < 2)
			cnt_max = 2;
	}
	else {

		if (cur_mode != 1)
			set_led_fun(1);

		cnt_max = (150 - am->acc[CWIID_X]) / 2;
		if (cnt_max < 2)
			cnt_max = 2;
	}
}

void handle_acc_draw(cwiid_wiimote_t *wiimote, struct cwiid_acc_mesg *am,
	struct timespec *ts)
{
	static int8_t pos_x = 0, pos_y = 0, pos_z = 0;
	static float rel_x = 0, rel_y = 0, rel_z = 0;
	static float start_x = 0, start_y = 0, start_z = 0;

	float ftime = (((float)ts->tv_nsec * 1000000000) + ts->tv_sec);

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

	if ((accel > 0.95) || (accel < 1.05))
		rel_x = rel_y = rel_z = 0;

	if ((a_x < -0.1) || (a_x > 0.1)) {
		if (!start_x)
			start_x = ftime;
		rel_x += a_x * -10;

		if ((ftime - start_x) > 0.2) {
			start_x = ftime;
			pos_x += rel_x;
		}
	}
	if ((a_y < -0.1) || (a_y > 0.1)) {
		if (!start_y)
			start_y = ftime;
		rel_y += a_y * -10;

		if ((ftime - start_y) > 0.2) {
			start_y = ftime;
			pos_y += rel_y;
		}
	}

	if (pos_x < 0) {
		pos_x = 0;
		rel_x = 0;
	}
	if (pos_x > 100) {
		pos_x = 100;
		rel_x = 0;
	}
	if (pos_y < 0) {
		pos_y = 0;
		rel_y = 0;
	}
	if (pos_y > 100) {
		pos_y = 100;
		rel_y = 0;
	}
	if (pos_z < 0) {
		pos_z = 0;
		rel_z = 0;
	}
	if (pos_z > 100) {
		pos_z = 100;
		rel_z = 0;
	}

	printf("\r\033[2KX=%3d Y=%3d Z=%3d %5.2f %5.2f %5.2f  roll=%5.2f pitch=%5.2f",
		pos_x, pos_y, pos_z,
		a_x - (roll / 1.53), a_y - (pitch / 1.57), a_z - (roll - 1.53) / 1.53,
		roll, pitch);
	fflush(stdout);
}

void handle_mp_draw(cwiid_wiimote_t *wiimote, struct cwiid_motionplus_mesg *mm)
{
	static char space[3600];
	static char was_drawing = 0;
	static int pos = 0, pos_old = 0;;

	int delta = mm->angle_rate[2] - 8230;

	if (abs(delta) > 50)
		pos = (pos + (delta / 180) + 3600) % 3600;

		if (drawing && was_drawing) {
			if (abs(pos_old - pos) < 2000)
				for (int i = pos_old; i != pos;
						i = (i + (pos_old < pos ? 1 : -1) + 3600) % 3600)
					space[i] = ((drawing == 1) ? 1 : 0);
			else
				for (int i = pos_old; i != pos;
						i = (i + (pos_old > pos ? 1 : -1) + 3600) % 3600)
					space[i] = ((drawing == 1) ? 1 : 0);
		}


	if ((space[(uint16_t)pos] && (drawing != -1)) || (drawing == 1))
		cwiid_set_led(wiimote, 0xf);
	else
		cwiid_set_led(wiimote, 0x0);

	pos_old = pos;
	was_drawing = drawing;
}

void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
	union cwiid_mesg mesg[], struct timespec *ts)
{
	for (int i = 0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_BTN) {

			if (mesg[i].btn_mesg.buttons & CWIID_BTN_UP) {
				set_next_mode();
				switch (opmode) {
					case AUTO_MODE:
					x_max = X_MAXP;
					puts("\r\033[2KAuto Mode");
					puts("- Shake to vibrate");
					puts("- Down arrow to toggle vibration");
					puts("- Tilt to change animation speed");
					puts("- Let wiimote fall to calculate fall height\n");

					sleep(1);
					if (cwiid_disable(wiimote, CWIID_FLAG_MOTIONPLUS))
						fputs("Unable to disable motionplus O.o", stderr);

					break;

					case BLINKEN_MODE:
					puts("\r\033[2KBlinken Mode");
					puts("- Down arrow to vibrate");
					puts("- Left/Right to change animation");
					puts("- +/- to change animation speed\n");
					break;

					case DRAW_MODE:
					puts("\r\033[2KAcctest (will become draw mode)\n");
					break;

					case DRAWCIRCLE_MODE:
					puts("\r\033[2KCircle Draw Mode");
					puts("- Hold 1 or A to draw");
					puts("- Hold 2 or B to erase\n");

					sleep(1);
					if (cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS))
						fputs("Unable to enable motionplus!", stderr);

					break;

					case INVAL_MODE:
					puts("\r\033[2KThis should never happen.");
					break;
				}
			}
			else if (opmode == DRAWCIRCLE_MODE)
				handle_button_draw(wiimote, mesg[i].btn_mesg.buttons);
			else
				handle_button_normal(wiimote, mesg[i].btn_mesg.buttons);

			if (mesg[i].btn_mesg.buttons & CWIID_BTN_HOME) {
				exit(0);
			}

		}
		else if (mesg[i].type == CWIID_MESG_ACC) {
			if (opmode == AUTO_MODE)
				handle_acc_auto(wiimote, &mesg[i].acc_mesg, ts);
			else if (opmode == DRAW_MODE)
				handle_acc_draw(wiimote, &mesg[i].acc_mesg, ts);
		}
		else if (mesg[i].type == CWIID_MESG_MOTIONPLUS) {
			if (opmode == DRAWCIRCLE_MODE)
				handle_mp_draw(wiimote, &mesg[i].motionplus_mesg);
		}
	}
}

