#include "logic.h"

#include <stdlib.h>

#include "audio.h"
#include "common.h"
#include "director.h"
#include "player.h"

#define HIT_WINDOW_PERFECT 43
#define HIT_WINDOW_GOOD 91
#define HIT_WINDOW_OK 139
#define HIT_WINDOW_MISS (HIT_WINDOW_OK * 2)

#define SCORE_PERFECT 300
#define SCORE_GOOD 100
#define SCORE_OK 50
#define SCORE_MISS 0

#define COMBO_BASE_MULT 1
#define COMBO_A_GATE 10
#define COMBO_A_MULT 2
#define COMBO_B_GATE 20
#define COMBO_B_MULT 4
#define COMBO_C_GATE 40
#define COMBO_C_MULT 8

#define HEALTH_INITIAL 5
#define HEALTH_MAX 5
#define HEALTH_INV_TIME (HIT_WINDOW_MISS * 3)
#define HEALTH_REGEN_TIME_MULT 2.5

static unsigned long last_time;

static Note *next_note_to_hit;
static unsigned int remaining_notes_to_hit;
static unsigned int notes_passed;

static int global_offset;

static unsigned long score;
static unsigned int hits_perfect;
static unsigned int hits_good;
static unsigned int hits_ok;
static unsigned int hits_miss;

static unsigned int combo;
static unsigned short combo_multiplier;

static unsigned short health;
static unsigned int remaining_invencibility;
static unsigned int health_regen_time;
static unsigned int time_to_health_regen;
static bool has_failed;

HitAssessment top_hit_assessment;
HitAssessment bottom_hit_assessment;

static void increase_score(int increment) {
  score += increment * combo_multiplier + combo / 10;
}

static void increase_combo(void) {
  combo += 1;

  switch (combo) {
  case COMBO_A_GATE:
    combo_multiplier = COMBO_A_MULT;
    break;

  case COMBO_B_GATE:
    combo_multiplier = COMBO_B_MULT;
    break;

  case COMBO_C_GATE:
    combo_multiplier = COMBO_C_MULT;
    break;

  default: {
  }
  }
}

static void reset_combo(void) {
  combo = 0;
  combo_multiplier = COMBO_BASE_MULT;
}

static void take_damage(void) {
  if (health > 0 && remaining_invencibility == 0) {
    printf("HEALTH (-1) %hu -> %hu\n", health, health - 1);

    health -= 1;
    remaining_invencibility = HEALTH_INV_TIME;
    time_to_health_regen = health_regen_time;
  }
}

static bool check_note(void) {
  if (remaining_notes_to_hit <= 0) {
    return false;
  }

  unsigned long press_position = audioPlaybackPosition();
  unsigned long expected_position = next_note_to_hit->position;

  long diff = press_position + global_offset - expected_position;
  unsigned long abs_diff = labs(diff);

  printf("HIT %ldms... ", diff);

  HitAssessment *hit_assessment =
      next_note_to_hit->topLane ? &top_hit_assessment : &bottom_hit_assessment;
  if (abs_diff <= HIT_WINDOW_MISS) {
    next_note_to_hit->hidden = true;

    hit_assessment->valid = true;
    hit_assessment->press_position = press_position;
    hit_assessment->expected_position = expected_position;

    next_note_to_hit++;
    notes_passed++;
    remaining_notes_to_hit--;
  }

  if (abs_diff <= HIT_WINDOW_PERFECT) {
    printf("PERFECT.\n");

    hit_assessment->valuation = HIT_VAL_PERFECT;

    increase_score(SCORE_PERFECT);
    increase_combo();

    hits_perfect++;
  } else if (abs_diff <= HIT_WINDOW_GOOD) {
    printf("GOOD.\n");

    hit_assessment->valuation = HIT_VAL_GOOD;

    increase_score(SCORE_GOOD);
    increase_combo();

    hits_good++;
  } else if (abs_diff <= HIT_WINDOW_OK) {
    printf("OK.\n");

    hit_assessment->valuation = HIT_VAL_OK;

    increase_score(SCORE_OK);
    increase_combo();

    hits_ok++;
  } else if (abs_diff <= HIT_WINDOW_MISS) {
    printf("MISS.\n");

    hit_assessment->valuation = HIT_VAL_MISS;

    increase_score(SCORE_MISS);
    take_damage();
    reset_combo();

    hits_miss++;
  } else {
    printf("ignored.\n");
    return false;
  }

  return true;
}

static void action_down(void) {
  if (player_current_lane() == LANE_BOTTOM) {
    player_hit();
  } else {
    player_quick_fall();
  }

  if (!next_note_to_hit->topLane) {
    check_note();
  } else {
    printf("HIT wrong lane.\n");
  }
}

static void action_up(void) {
  if (player_current_lane() == LANE_TOP) {
    player_hit();
  }

  if (next_note_to_hit->topLane) {
    if (check_note()) {
      player_quick_jump();
    } else {
      player_jump();
    }
  } else {
    printf("HIT wrong lane.\n");
    player_jump();
  }
}

static void advance_notes(void) {
  long long last_chance_position = 0;
  if (audioPlaybackPosition() > HIT_WINDOW_MISS) {
    last_chance_position = (signed long long)audioPlaybackPosition() +
                           global_offset - HIT_WINDOW_MISS;
  }

  while (remaining_notes_to_hit > 0 &&
         next_note_to_hit->position < last_chance_position) {
    printf("Lost chance for note at %lu (< %lld)\n", next_note_to_hit->position,
           last_chance_position);

    next_note_to_hit++;
    notes_passed++;
    remaining_notes_to_hit--;

    take_damage();
    reset_combo();
  }
}

static void update_health(unsigned int dt) {
  time_to_health_regen =
      saturated_sub_lu(time_to_health_regen, (unsigned long)dt);
  remaining_invencibility =
      saturated_sub_lu(remaining_invencibility, (unsigned long)dt);

  if (health > 0 && health < HEALTH_MAX && time_to_health_regen == 0) {
    printf("HEALTH (+1) %hu -> %hu\n", health, health + 1);

    health += 1;
    time_to_health_regen = health_regen_time;
  }
}

void logic_init() {
  last_time = 0;

  next_note_to_hit = beatmap->notes;
  remaining_notes_to_hit = beatmap->note_count;
  global_offset = beatmap->start_offset;

  score = 0;
  notes_passed = 0;
  reset_combo();

  health = HEALTH_INITIAL;
  remaining_invencibility = 0;
  health_regen_time =
      (unsigned int)(beatmap->approach_time * HEALTH_REGEN_TIME_MULT);
  time_to_health_regen = 0;
  has_failed = false;

  hits_perfect = 0;
  hits_good = 0;
  hits_ok = 0;
  hits_miss = 0;

  top_hit_assessment.valid = false;
  bottom_hit_assessment.valid = false;
}

void logic_end(void) {
  free(beatmap);
  audioStop();
}

void logic_update(unsigned int dt) {
  player_update(dt);

  u32 k_down = hidKeysDown();

  /* Escape to menu */
  if (k_down & KEY_SELECT) {
    director_change_state(SONG_SELECTION_MENU);
  }

  /* Pause on START */
  if ((k_down & KEY_START) && !logic_has_failed()) {
    printf("\n** Toggle pause **\n");

    if (audioIsPaused()) {
      audioPlay();
    } else {
      audioPause();
    }
  }

  if (!audioAdvancePlaybackPosition()) {
    return;
  }

  advance_notes();

  update_health(dt);
  if (health == 0) {
    printf("--- FAIL ---\n");
    has_failed = true;
    audioPause();
  }

#ifdef DEBUG_AUTO
  if (saturated_sub_lu(next_note_to_hit->position, audioPlaybackPosition()) ==
      0) {
    if (next_note_to_hit->topLane) {
      action_up();
    } else {
      action_down();
    }
  }
#else
  if (k_down & KEY_A || k_down & KEY_B || k_down & KEY_DOWN || k_down & KEY_R) {
    action_down();
  }
  if (k_down & KEY_X || k_down & KEY_Y || k_down & KEY_UP || k_down & KEY_L) {
    action_up();
  }
#endif
}

unsigned long logic_score(void) { return score; }

unsigned int logic_combo(void) { return combo; }

bool logic_is_full_combo(void) { return combo == notes_passed; }

unsigned short logic_current_health(void) { return health; }

unsigned short logic_max_health(void) { return HEALTH_MAX; }

bool logic_is_invencible(void) { return remaining_invencibility > 0; }

bool logic_has_failed(void) { return has_failed; }

HitAssessment logic_top_hit_assessment(void) { return top_hit_assessment; }

HitAssessment logic_bottom_hit_assessment(void) {
  return bottom_hit_assessment;
}

unsigned int logic_hit_count_perfect(void) { return hits_perfect; }

unsigned int logic_hit_count_good(void) { return hits_good; }

unsigned int logic_hit_count_ok(void) { return hits_ok; }

unsigned int logic_hit_count_miss(void) { return hits_miss; }

bool logic_is_finished(void) { return remaining_notes_to_hit == 0; }
