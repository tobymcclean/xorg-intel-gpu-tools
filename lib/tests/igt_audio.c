/*
 * Copyright © 2019 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Author: Simon Ser <simon.ser@intel.com>
 */

#include "config.h"

#include <stdlib.h>

#include "igt_core.h"
#include "igt_audio.h"

#define SAMPLING_RATE 44100
#define CHANNELS 1
#define BUFFER_LEN 2048

static const int test_freqs[] = { 300, 700, 5000 };

static const size_t test_freqs_len = sizeof(test_freqs) / sizeof(test_freqs[0]);

#define TEST_EXTRA_FREQ 500

static void test_signal_detect_untampered(struct audio_signal *signal)
{
	double buf[BUFFER_LEN];
	bool ok;

	audio_signal_fill(signal, buf, BUFFER_LEN / CHANNELS);
	ok = audio_signal_detect(signal, SAMPLING_RATE, 0, buf, BUFFER_LEN);
	igt_assert(ok);
}

static void test_signal_detect_silence(struct audio_signal *signal)
{
	double buf[BUFFER_LEN] = {0};
	bool ok;

	ok = audio_signal_detect(signal, SAMPLING_RATE, 0, buf, BUFFER_LEN);

	igt_assert(!ok);
}

static void test_signal_detect_noise(struct audio_signal *signal)
{
	double buf[BUFFER_LEN];
	bool ok;
	size_t i;

	/* Generate random samples between -1 and 1 */
	srand(42);
	for (i = 0; i < BUFFER_LEN; i++)
		buf[i] = (double) random() / RAND_MAX * 2 - 1;

	ok = audio_signal_detect(signal, SAMPLING_RATE, 0, buf, BUFFER_LEN);

	igt_assert(!ok);
}

static void test_signal_detect_with_missing_freq(struct audio_signal *signal)
{
	double buf[BUFFER_LEN];
	struct audio_signal *missing;
	bool ok;
	size_t i;

	/* Generate a signal with all the expected frequencies but the first
	 * one */
	missing = audio_signal_init(CHANNELS, SAMPLING_RATE);
	for (i = 1; i < test_freqs_len; i++) {
		audio_signal_add_frequency(missing, test_freqs[i], 0);
	}
	audio_signal_synthesize(missing);

	audio_signal_fill(missing, buf, BUFFER_LEN / CHANNELS);
	ok = audio_signal_detect(signal, SAMPLING_RATE, 0, buf, BUFFER_LEN);
	igt_assert(!ok);
}

static void test_signal_detect_with_unexpected_freq(struct audio_signal *signal)
{
	double buf[BUFFER_LEN];
	struct audio_signal *extra;
	bool ok;
	size_t i;

	/* Add an extra, unexpected frequency */
	extra = audio_signal_init(CHANNELS, SAMPLING_RATE);
	for (i = 0; i < test_freqs_len; i++) {
		audio_signal_add_frequency(extra, test_freqs[i], 0);
	}
	audio_signal_add_frequency(extra, TEST_EXTRA_FREQ, 0);
	audio_signal_synthesize(extra);

	audio_signal_fill(extra, buf, BUFFER_LEN / CHANNELS);
	ok = audio_signal_detect(signal, SAMPLING_RATE, 0, buf, BUFFER_LEN);
	igt_assert(!ok);
}

igt_main
{
	struct audio_signal *signal;
	int ret;
	size_t i;

	igt_subtest_group {
		igt_fixture {
			signal = audio_signal_init(CHANNELS, SAMPLING_RATE);

			for (i = 0; i < test_freqs_len; i++) {
				ret = audio_signal_add_frequency(signal,
								 test_freqs[i],
								 0);
				igt_assert(ret == 0);
			}

			audio_signal_synthesize(signal);
		}

		igt_subtest("signal-detect-untampered")
			test_signal_detect_untampered(signal);

		igt_subtest("signal-detect-silence")
			test_signal_detect_silence(signal);

		igt_subtest("signal-detect-noise")
			test_signal_detect_noise(signal);

		igt_subtest("signal-detect-with-missing-freq")
			test_signal_detect_with_missing_freq(signal);

		igt_subtest("signal-detect-with-unexpected-freq")
			test_signal_detect_with_unexpected_freq(signal);

		igt_fixture {
			audio_signal_fini(signal);
		}
	}
}