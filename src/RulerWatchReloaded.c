#include <pebble.h>

#define RULER_XOFFSET 24
#define LINE_YPOS 83
#define ANIM_DELAY 1000
#define ANIM_DURATION 2000
#define LINE_INTERVAL 5
#define TEXT_OFFSET 5
#define MARK_0  42
#define MARK_5  12
#define MARK_15 22
#define MARK_30 32
#define NUM_LABELS 59

enum {
  CONFIG_KEY_INVERT		= 1010,
  CONFIG_KEY_VIBRATION	= 1011
};

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static int hour_size = 12 * LINE_INTERVAL; // 12 marks, one every 5 minutes
static int hour_part_size;
static GRect rect_text = { { RULER_XOFFSET, 0 }, { 60, 60 } };
static GPoint line1_p1 = { 0, LINE_YPOS };
static GPoint line1_p2 = { 143, LINE_YPOS };
static GPoint line2_p1;
static GPoint line2_p2;
static GPoint mark1 = { 24, 0 };
static GPoint mark2 = { 0, 0 };

static int markWidth[12] = { MARK_0, MARK_5, MARK_5, MARK_15, MARK_5, MARK_5, MARK_30, MARK_5, MARK_5, MARK_15, MARK_5, MARK_5 };

static int labels_12h[NUM_LABELS] = { 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 1 };
static int labels[NUM_LABELS] = { 22, 23, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 1 };

static struct tm now;

static int invert = false;
static int vibration = false;

static GColor COLOR_FOREGROUND = GColorBlack;
static GColor COLOR_BACKGROUND = GColorWhite;

static char text[3] = "  ";

static uint32_t anim_time = 0;
static int anim_phase = 0;

static bool animRunning = false;

static AnimationImplementation animImpl;
static Animation *anim;

static void drawDial(GContext *ctx) {
  if (invert) {
    graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);

    graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);
    graphics_draw_line(ctx, line1_p1, line1_p2);
    graphics_draw_line(ctx, line2_p1, line2_p2);

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
  } else {
    graphics_context_set_fill_color(ctx, COLOR_FOREGROUND);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
    graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
    graphics_fill_rect(ctx, GRect(5, 5, 134, LINE_YPOS-6) , 4, GCornersAll);
    graphics_fill_rect(ctx, GRect(5, LINE_YPOS+1, 134, 168-LINE_YPOS-6) , 4, GCornersAll);

    graphics_context_set_compositing_mode(ctx, GCompOpAnd);
  }
}

/*
 * 
 * Ruler format :
 * 
 *    <------------------HOURS----------------><---------------------DAYS---------------------->
 *    |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | |
 *    22 0  2  4  6  8  10 12 14 16 18 20 22 0  2  4  6  8  10 12 14 16 18 20 22 24 26 28 30 1 2
 *
 * Hours part is (hour_size * 26) pixels
 * Days part is (hour_size * 33) pixels
 * Total size is (hour_size * 59) pixels
 * 
 * In "normal" hour mode, position is depending only on the number of minutes since midnight
 * In "animRunning" day mode, position is depending on the day
 * 
 */

static void drawRuler(GContext *ctx) {
  unsigned int i, j;
  int hour_offset, day_offset, yh, yd, y;

  // Pixel offset of the middle of the screen
  // Offset for hour mode, starting at 22
  hour_offset = ((now.tm_hour+2) * hour_size) + (now.tm_min * hour_size / 60);
  // Offset for day mode
  day_offset = hour_part_size + now.tm_mday * hour_size;
  
  yd = LINE_YPOS - day_offset;
  yh = LINE_YPOS - hour_offset;
  
  if (animRunning) {
    if (anim_phase == 1) {
      // Phase 1, move from hour to day
      y = ((yd - yh) * (int)anim_time) / (int)ANIMATION_NORMALIZED_MAX + yh;
    } else {
      // Phase 2, move from day to hour
      y = ((yh - yd) * (int)anim_time) / (int)ANIMATION_NORMALIZED_MAX + yd;
    }
  } else {
    y = yh;
  }
    
  graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);
  for (i=0; i<NUM_LABELS; i++) {
    for (j=0; j<12; j++) {
      if ((y > -20) && (y < 188)) {
        if ((y >= 0) && (y < 168)) {
          mark1.y = mark2.y = y;
          mark2.x = mark1.x + markWidth[j];
          graphics_draw_line(ctx, mark1, mark2);
          mark1.y = mark2.y = y - 1;
          graphics_draw_line(ctx, mark1, mark2);
          mark1.y = mark2.y = y + 1;
          graphics_draw_line(ctx, mark1, mark2);
        }
        
        if (j == 0) {
          rect_text.origin.y = y - 32;
          snprintf(text, sizeof(text), "%d", labels[i]);
          graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
          graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49), rect_text, GTextOverflowModeWordWrap,
                             GTextAlignmentLeft, NULL);
        }
      }
      y += LINE_INTERVAL;
    }
  }
}

static void layer_update(Layer *me, GContext* ctx) {
  drawDial(ctx);
  drawRuler(ctx);
}

static void rescheduleAnim(struct Animation *anim) {
  anim_phase++;
  if (anim_phase == 2) {
    animation_schedule(anim);
  } else {
    anim_phase = 0;
    animRunning = false;
  }
}

static void handleAnim(struct Animation *anim, const uint32_t normTime) {
  anim_time = normTime;
  layer_mark_dirty(layer);
}

static void handle_tap(AccelAxisType axis, int32_t direction) {
  if (!animation_is_scheduled(anim)) {
    animRunning = true;
    anim_phase = 1;
    animation_schedule(anim);
  }
}

static void handle_tick(struct tm *cur, TimeUnits units_changed) {
  now = *cur;

  if (vibration && now.tm_min == 0) {
    vibes_short_pulse();
  }

  layer_mark_dirty(layer);
}

static void setColors() {
  if (invert) {
    COLOR_BACKGROUND = GColorBlack;
    COLOR_FOREGROUND = GColorWhite;
  } else {
    COLOR_BACKGROUND = GColorWhite;
    COLOR_FOREGROUND = GColorBlack;
  }
}

static void applyConfig() {
  setColors();
  layer_mark_dirty(layer);
}

static void logVariables(const char *msg) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MSG: %s\n\tinvert=%d\n\tvibration=%d\n", msg, invert, vibration);
}

static bool checkAndSaveInt(int *var, int val, int key) {
  if (*var != val) {
    *var = val;
    persist_write_int(key, val);
    return true;
  } else {
    return false;
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler reason = %d", (int)reason);
}

static void in_received_handler(DictionaryIterator *received, void *context) {
  bool somethingChanged = false;

  Tuple *invertTuple = dict_find(received, CONFIG_KEY_INVERT);
  Tuple *vibrationTuple = dict_find(received, CONFIG_KEY_VIBRATION);

  if (invertTuple && vibrationTuple) {
    somethingChanged |= checkAndSaveInt(&invert, invertTuple->value->int32, CONFIG_KEY_INVERT);
    somethingChanged |= checkAndSaveInt(&vibration, vibrationTuple->value->int32, CONFIG_KEY_VIBRATION);

    logVariables("ReceiveHandler");

    if (somethingChanged) {
      applyConfig();
    }
  }
}

static void readConfig() {
  if (persist_exists(CONFIG_KEY_INVERT)) {
    invert = persist_read_int(CONFIG_KEY_INVERT);
  } else {
    invert = 0;
  }

  if (persist_exists(CONFIG_KEY_VIBRATION)) {
    vibration = persist_read_int(CONFIG_KEY_VIBRATION);
  } else {
    vibration = 0;
  }

  logVariables("readConfig");
}

static void app_message_init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(64, 64);
}


static void init() {
  time_t t;
  int i, n;

  line2_p1 = line1_p1;
  line2_p2 = line1_p2;
  line2_p1.y++;
  line2_p2.y++;

  if (!clock_is_24h_style()) {
    for (i=0; i<NUM_LABELS; i++) {
      labels[i] = labels_12h[i];
    }
  }

  readConfig();
  setColors();

  app_message_init();

  window = window_create();
  window_set_background_color(window, COLOR_BACKGROUND);
  window_stack_push(window, true);

  rootLayer = window_get_root_layer(window);

  layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(layer, layer_update);
  layer_add_child(rootLayer, layer);

  rect_text.origin.x += MARK_0 + TEXT_OFFSET;
  hour_part_size = 26 * hour_size;

  t = time(NULL);
  now = *(localtime(&t));

  animImpl.setup = NULL;
  animImpl.update = handleAnim;
  animImpl.teardown = rescheduleAnim;

  anim = animation_create();
  animation_set_delay(anim, ANIM_DELAY);
  animation_set_duration(anim, ANIM_DURATION);
  animation_set_implementation(anim, &animImpl);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  accel_tap_service_subscribe(handle_tap);
}


static void deinit() {
  if (animRunning) {
    animation_unschedule(anim);
  }
  animation_destroy(anim);
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  layer_destroy(layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
