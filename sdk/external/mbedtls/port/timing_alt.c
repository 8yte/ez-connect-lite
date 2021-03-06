/*
 *  Copyright 2008-2017, Marvell International Ltd.
 *  All Rights Reserved
 */

#include "timing_alt.h"
#include <wm_os.h>
#include <wm_mbedtls_debug.h>

volatile int mbedtls_timing_alarmed;

unsigned long mbedtls_timing_hardclock(void)
{
	time_t cycles = os_get_timestamp();

	return (unsigned long) cycles;
}

unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val,
		int reset)
{
	unsigned long delta;
	struct mbedtls_timing_hr_time offset;

	gettimeofday(&offset, NULL);

	if (reset) {
		val->tv_sec  = offset.tv_sec;
		val->tv_usec = offset.tv_usec;
		return 0;
	}

	delta = (offset.tv_sec  - val->tv_sec) * 1000
		+ (offset.tv_usec - val->tv_usec) / 1000;

	return delta;
}

static void mbedtls_one_shot_timer_cb(os_timer_arg_t arg)
{
	wm_mbedtls_d("One Shot timer callback");
	/* Set value to zero at end */
	mbedtls_timing_alarmed = 1;
}

void mbedtls_set_alarm(int seconds)
{
	/* Set value to zero at start */
	mbedtls_timing_alarmed = 0;

	os_timer_t one_shot_timer;
	/* Create one shot timer */
	wm_mbedtls_d("One Shot Timer Created");
	int rv = os_timer_create(&one_shot_timer,
			"mbedtls-oneshot-timer",
			os_msec_to_ticks(seconds),
			&mbedtls_one_shot_timer_cb,
			NULL,
			OS_TIMER_ONE_SHOT,
			OS_TIMER_NO_ACTIVATE);

	if (rv != WM_SUCCESS) {
		wm_mbedtls_e("Unable to create timer");
		return;
	}

	wm_mbedtls_d("Activate the timer");
	/* Activate one shot timer */
	os_timer_activate(&one_shot_timer);
	return;
}

int mbedtls_timing_get_delay(void *data)
{
	mbedtls_timing_delay_context *ctx =
		(mbedtls_timing_delay_context *) data;
	unsigned long elapsed_ms;

	if (ctx->fin_ms == 0)
		return -1;

	elapsed_ms = mbedtls_timing_get_timer(&ctx->timer, 0);

	if (elapsed_ms >= ctx->fin_ms)
		return 2;

	if (elapsed_ms >= ctx->int_ms)
		return 1;

	return 0;
}

void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
	mbedtls_timing_delay_context *ctx =
		(mbedtls_timing_delay_context *) data;

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if (fin_ms != 0)
		(void) mbedtls_timing_get_timer(&ctx->timer, 1);
}
