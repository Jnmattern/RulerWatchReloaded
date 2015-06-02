#include <pebble.h>

#define RULER_XOFFSET 24
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
  CONFIG_KEY_INVERT =       1010,
  CONFIG_KEY_VIBRATION =    1011,
  CONFIG_KEY_LEGACY =       1012,
  CONFIG_KEY_BATTERY =      1013,
  CONFIG_KEY_DATEONSHAKE =  1014
};

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static int hour_size = 12 * LINE_INTERVAL; // 12 marks, one every 5 minutes
static int hour_part_size;
static GRect rect_text = { { RULER_XOFFSET, 0 }, { 60, 60 } };
static GPoint line1_p1 = { 0, 84 };
static GPoint line1_p2 = { 143, 84 };
static GPoint line2_p1;
static GPoint line2_p2;
static GPoint mark1 = { 24, 0 };
static GPoint mark2 = { 0, 0 };
static int hour_line_ypos = 84;

static int markWidth[12] = { MARK_0, MARK_5, MARK_5, MARK_15, MARK_5, MARK_5, MARK_30, MARK_5, MARK_5, MARK_15, MARK_5, MARK_5 };

static int labels_12h[NUM_LABELS] = { 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 1 };
static int labels[NUM_LABELS] = { 22, 23, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 1 };

static struct tm now;

static int invert = false;
static int vibration = false;
static int legacy = false;
static int battery = false;
static int dateonshake = true;

static GColor fgColor;
static GColor bgColor;

static uint8_t battery_level;
static GFont battery_gauge_font;

static const GPathInfo battery_mark_upper_path_info = {
  .num_points = 5,
  .points = (GPoint []) {{0, 0}, {8, 0}, {8, 4}, {4, 8}, {0, 4}}
};
static const GPathInfo battery_mark_lower_path_info = {
  .num_points = 5,
  .points = (GPoint []) {{4, 14}, {8, 18}, {8, 22}, {0, 22}, {0, 18}}
};
static GPath *battery_mark_upper_path = NULL;
static GPath *battery_mark_lower_path = NULL;


static char text[3] = "  ";

static AnimationProgress anim_time = 0;
static int anim_phase = 0;

static bool animRunning = false;

static AnimationImplementation animImpl;
static Animation *anim;

static void drawDial(GContext *ctx) {
  graphics_context_set_fill_color(ctx, fgColor);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
  graphics_context_set_fill_color(ctx, bgColor);
  graphics_fill_rect(ctx, GRect(5, 5, 134, hour_line_ypos-6), 4, GCornersAll);
  graphics_fill_rect(ctx, GRect(5, hour_line_ypos+1, 134, hour_line_ypos-6), 4, GCornersAll);
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
  
  yd = hour_line_ypos - day_offset;
  yh = hour_line_ypos - hour_offset;
  
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

  graphics_context_set_stroke_color(ctx, fgColor);
  for (i=0; i<NUM_LABELS; i++) {
    for (j=0; j<12; j++) {
      if ((y > -20) && (y < 188)) {
        if ((y >= 0) && (y < 168)) {
          mark1.y = mark2.y = y;
          mark2.x = mark1.x + markWidth[j];
          graphics_draw_line(ctx, mark1, mark2);
          if (!legacy) {
            mark1.y = mark2.y = y - 1;
            graphics_draw_line(ctx, mark1, mark2);
            mark1.y = mark2.y = y + 1;
            graphics_draw_line(ctx, mark1, mark2);
          }
        }
        
        if (j == 0) {
          snprintf(text, sizeof(text), "%d", labels[i]);
          graphics_context_set_text_color(ctx, fgColor);
          if (legacy) {
            rect_text.origin.y = y - 19;
            graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), rect_text, GTextOverflowModeWordWrap,
                               GTextAlignmentLeft, NULL);
          } else {
            rect_text.origin.y = y - 32;
            graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49), rect_text, GTextOverflowModeWordWrap,
                               GTextAlignmentLeft, NULL);
          }
        }
      }
      y += LINE_INTERVAL;
    }
  }
}

static void drawBattery(GContext *ctx) {
  int i, x;
  static char t[11][3] = { "E", "10", "20", "30", "40", "50", "60", "70", "80", "90", "F" };

  GPoint battery_mark_pos = GPoint(7+((int)battery_level*12/10), 144);

  // Battery Gauge Background
  graphics_context_set_fill_color(ctx, bgColor);
  graphics_fill_rect(ctx, GRect(5, 149, 134, 14), 4, GCornersAll);

  // Battery line and graduations
  graphics_context_set_stroke_color(ctx, fgColor);
  graphics_draw_line(ctx, GPoint(11, 161), GPoint(131, 161));
  for (i=0, x=11; i<=10; i++, x+=12) {
    graphics_draw_line(ctx, GPoint(x, 159), GPoint(x, 161));
    graphics_draw_text	(ctx, t[i], battery_gauge_font, GRect(x-9, 150, 20, 11), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  // Battery mark
  graphics_context_set_stroke_color(ctx, fgColor);
  graphics_context_set_fill_color(ctx, bgColor);
  gpath_move_to(battery_mark_upper_path, battery_mark_pos);
  gpath_move_to(battery_mark_lower_path, battery_mark_pos);
  gpath_draw_filled(ctx, battery_mark_upper_path);
  gpath_draw_filled(ctx, battery_mark_lower_path);
  gpath_draw_outline(ctx, battery_mark_upper_path);
  gpath_draw_outline(ctx, battery_mark_lower_path);
}

static void layer_update(Layer *me, GContext* ctx) {
  drawDial(ctx);
  drawRuler(ctx);
  if (battery) {
    drawBattery(ctx);
  }
}

static void createAnim();
static void destroyAnim();

static void rescheduleAnim(Animation *anim, bool finished, void *ctx) {
  destroyAnim();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "-> rescheduleAnim: phase=%d - animRunning = %d", anim_phase, animRunning);

  anim_phase++;
  if (anim_phase == 2) {
    anim_time = ANIMATION_NORMALIZED_MIN;
    createAnim();
    animation_schedule(anim);
  } else {
    anim_phase = 0;
    animRunning = false;
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "<- rescheduleAnim: phase=%d - animRunning = %d", anim_phase, animRunning);
}

static void handleAnim(struct Animation *anim, const AnimationProgress normTime) {
  anim_time = normTime;
  layer_mark_dirty(layer);
}

static void destroyAnim() {
#ifdef PBL_PLATFORM_APLITE
  if (anim != NULL) {
    animation_destroy(anim);
  }
#endif
  anim = NULL;
}

static void createAnim() {
  animImpl.setup = NULL;
  animImpl.update = (AnimationUpdateImplementation)handleAnim;
  animImpl.teardown = NULL;

  anim = animation_create();
  animation_set_delay(anim, ANIM_DELAY);
  animation_set_duration(anim, ANIM_DURATION);
  animation_set_handlers	(	anim, (AnimationHandlers){
    .started = NULL,
    .stopped = rescheduleAnim
    },
    NULL);
  animation_set_implementation(anim, &animImpl);
}


static void handle_tap(AccelAxisType axis, int32_t direction) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_tap: animRunning = %d", animRunning);
  if (dateonshake && (anim_phase == 0)) {
    animRunning = true;
    anim_phase = 1;
    createAnim();
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

static void handle_battery(BatteryChargeState charge) {
  battery_level = charge.charge_percent;
  // Don't force display update, will be done automatically on next minute change
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_battery -> %d", (int)battery_level);
}

static void setColors() {
  if (invert) {
    bgColor = GColorBlack;
    fgColor = GColorWhite;
  } else {
    bgColor = GColorWhite;
    fgColor = GColorBlack;
  }
}

static void setHourLinePoints() {
  if (battery) {
    hour_line_ypos = 73;
  } else {
    hour_line_ypos = 84;
  }

  line1_p1.y = line1_p2.y = hour_line_ypos;
  line2_p1 = line1_p1;
  line2_p2 = line1_p2;
  line2_p1.y++;
  line2_p2.y++;
}

static void applyConfig() {
  setColors();
  setHourLinePoints();
  layer_mark_dirty(layer);
}

static void logVariables(const char *msg) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MSG: %s\n\tinvert=%d\n\tvibration=%d\n\tlegacy=%d\n\tbattery=%d\n\tdateonshake=%d\n", msg, invert, vibration, legacy, battery, dateonshake);
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
  Tuple *legacyTuple = dict_find(received, CONFIG_KEY_LEGACY);
  Tuple *batteryTuple = dict_find(received, CONFIG_KEY_BATTERY);
  Tuple *dateonshakeTuple = dict_find(received, CONFIG_KEY_DATEONSHAKE);

  if (invertTuple && vibrationTuple && legacyTuple && batteryTuple && dateonshakeTuple) {
    somethingChanged |= checkAndSaveInt(&invert, invertTuple->value->int32, CONFIG_KEY_INVERT);
    somethingChanged |= checkAndSaveInt(&vibration, vibrationTuple->value->int32, CONFIG_KEY_VIBRATION);
    somethingChanged |= checkAndSaveInt(&legacy, legacyTuple->value->int32, CONFIG_KEY_LEGACY);
    somethingChanged |= checkAndSaveInt(&battery, batteryTuple->value->int32, CONFIG_KEY_BATTERY);
    somethingChanged |= checkAndSaveInt(&dateonshake, dateonshakeTuple->value->int32, CONFIG_KEY_DATEONSHAKE);

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

  if (persist_exists(CONFIG_KEY_LEGACY)) {
    legacy = persist_read_int(CONFIG_KEY_LEGACY);
  } else {
    legacy = 0;
  }

  if (persist_exists(CONFIG_KEY_BATTERY)) {
    battery = persist_read_int(CONFIG_KEY_BATTERY);
  } else {
    battery = 0;
  }

  if (persist_exists(CONFIG_KEY_DATEONSHAKE)) {
    dateonshake = persist_read_int(CONFIG_KEY_DATEONSHAKE);
  } else {
    dateonshake = 1;
  }

  logVariables("readConfig");
}

static void app_message_init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(128, 128);
}

static void initBatteryLevel() {
  BatteryChargeState battery_state = battery_state_service_peek();

  battery_level = battery_state.charge_percent;
}

static void initBatteryMarkPaths() {
  battery_mark_upper_path = gpath_create(&battery_mark_upper_path_info);
  battery_mark_lower_path = gpath_create(&battery_mark_lower_path_info);
}

static void destroyBatteryMarkPaths() {
  gpath_destroy(battery_mark_upper_path);
  gpath_destroy(battery_mark_lower_path);
}

static void init() {
  time_t t;
  int i;

  if (!clock_is_24h_style()) {
    for (i=0; i<NUM_LABELS; i++) {
      labels[i] = labels_12h[i];
    }
  }

  fgColor = GColorWhite;
  bgColor = GColorBlack;

  readConfig();
  setColors();
  setHourLinePoints();
  initBatteryLevel();

  app_message_init();

  battery_gauge_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BATTERY_GAUGE_SUBSET_8));

  initBatteryMarkPaths();

  window = window_create();
  window_set_background_color(window, bgColor);
  window_stack_push(window, true);

  rootLayer = window_get_root_layer(window);

  layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(layer, layer_update);
  layer_add_child(rootLayer, layer);

  rect_text.origin.x += MARK_0 + TEXT_OFFSET;
  hour_part_size = 26 * hour_size;

  t = time(NULL);
  now = *(localtime(&t));

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  accel_tap_service_subscribe(handle_tap);
  battery_state_service_subscribe(handle_battery);
}


static void deinit() {
  if (animRunning) {
    animation_unschedule(anim);
  }
  animation_destroy(anim);
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_destroy(layer);
  window_destroy(window);

  destroyBatteryMarkPaths();

  fonts_unload_custom_font(battery_gauge_font);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
