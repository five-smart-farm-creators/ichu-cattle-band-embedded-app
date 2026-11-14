// Wokwi custom chip: photoelectric pulse sensor (SP-203 style) with an analog PPG output.
#include "wokwi-api.h"
#include <math.h>
#include <stdlib.h>

#define SAMPLE_PERIOD_US 10000
#define BASELINE_V 1.65f
#define PEAK_V 0.9f
#define NOTCH_V 0.25f

typedef struct {
  pin_t out;
  uint32_t bpm_attr;
  float phase;
} chip_state_t;

static float gaussian(float x, float center, float width) {
  float d = (x - center) / width;
  return expf(-0.5f * d * d);
}

static void on_sample(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  float bpm = attr_read_float(chip->bpm_attr);
  if (bpm <= 0.0f) {
    // No skin contact: the real module idles near mid-rail with no pulse.
    pin_dac_write(chip->out, BASELINE_V);
    return;
  }
  chip->phase += (bpm / 60.0f) * (SAMPLE_PERIOD_US / 1e6f);
  if (chip->phase >= 1.0f) chip->phase -= 1.0f;

  // Systolic peak plus a smaller dicrotic wave, like a fingertip PPG trace.
  float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.03f;
  float volts = BASELINE_V + PEAK_V * gaussian(chip->phase, 0.15f, 0.05f)
              + NOTCH_V * gaussian(chip->phase, 0.40f, 0.07f) + noise;
  pin_dac_write(chip->out, volts);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->out = pin_init("OUT", ANALOG);
  chip->bpm_attr = attr_init_float("bpm", 70.0f);
  chip->phase = 0.0f;

  const timer_config_t timer_config = { .user_data = chip, .callback = on_sample };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, SAMPLE_PERIOD_US, true);
}
