/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zmk/debounce.h>

#ifndef CONFIG_ZMK_DEBOUNCE_EAGER
static uint32_t get_threshold(const struct zmk_debounce_state *state,
                              const struct zmk_debounce_config *config) {
    return state->pressed ? config->debounce_release_ms : config->debounce_press_ms;
#else
static uint32_t get_counter(const struct zmk_debounce_state *state,
                              const struct zmk_debounce_config *config) {
    return !state->pressed ? config->debounce_release_ms : config->debounce_press_ms;
#endif
}

static void increment_counter(struct zmk_debounce_state *state, const int elapsed_ms) {
    if (state->counter + elapsed_ms > DEBOUNCE_COUNTER_MAX) {
        state->counter = DEBOUNCE_COUNTER_MAX;
    } else {
        state->counter += elapsed_ms;
    }
}

static void decrement_counter(struct zmk_debounce_state *state, const int elapsed_ms) {
    if (state->counter < elapsed_ms) {
        state->counter = 0;
    } else {
        state->counter -= elapsed_ms;
    }
}

void zmk_debounce_update(struct zmk_debounce_state *state, const bool active, const int elapsed_ms,
                         const struct zmk_debounce_config *config) {
    // This uses a variation of the integrator debouncing described at
    // https://www.kennethkuhn.com/electronics/debounce.c
    // Every update where "active" does not match the current state, we increment
    // a counter, otherwise we decrement it. When the counter reaches a
    // threshold, the state flips and we reset the counter.
    // the difference between defer and eager debouncing lays
    // in swaping either the counter or the threshold.
    // while in the eager method if the counter is reset and we have a mismatch
    // the state flips and the counter is set to the
    // debounce delay, otherwise if the counter reaches the
    // sum of the two debounce delays, we reset the counter and flip the state.
    // in the defer method we set the threshold to the debounce delay and wait for the
    // counter to reach it in order to flip the state and reset the counter.
    // where the debounce delays are the minimal amount of time
    // it takes for a full state flip after a release/press.
    // this makes it so that eager first flips the state, and than flips back if
    // it is incorrect, while defer waits to be sure and than flips the state.
    state->changed = false;

    if (active == state->pressed) {
        decrement_counter(state, elapsed_ms);
        return;
    }

#ifndef CONFIG_ZMK_DEBOUNCE_EAGER
    const uint32_t flip_threshold = get_threshold(state, config);
#else
    const uint32_t flip_threshold = config->debounce_release_ms + config->debounce_press_ms;

    if (state->counter == 0) {
        state->counter = get_counter(state, config);
    } else
#endif
    if (state->counter < flip_threshold) {
        increment_counter(state, elapsed_ms);
        return;
    }
    else
        state->counter = 0;

    state->pressed = !state->pressed;
    state->changed = true;
}

bool zmk_debounce_is_active(const struct zmk_debounce_state *state) {
    return state->pressed || state->counter > 0;
}

bool zmk_debounce_is_pressed(const struct zmk_debounce_state *state) { return state->pressed; }

bool zmk_debounce_get_changed(const struct zmk_debounce_state *state) { return state->changed; }
