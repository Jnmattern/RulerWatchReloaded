#include <pebble.h>

#define RULER_XOFFSET 24
#define LINE_YPOS 83
#define ANIM_DURATION 6000

enum {
  CONFIG_KEY_INVERT		= 1010,
  CONFIG_KEY_VIBRATION	= 1011
};

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static GBitmap *ruler_bitmap;
static int hour_size = 0;
static GRect rect = { { RULER_XOFFSET, 0 }, { 0, 0 } };
static GRect rect_text = { { RULER_XOFFSET, 0 }, { 60, 40 } };
static GPoint line1_p1 = { 0, LINE_YPOS };
static GPoint line1_p2 = { 143, LINE_YPOS };
static GPoint line2_p1;
static GPoint line2_p2;

static struct tm now;

static int invert = false;
static int vibration = false;

static GColor COLOR_FOREGROUND = GColorBlack;
static GColor COLOR_BACKGROUND = GColorWhite;

static char text[3] = "  ";

static uint32_t anim_time = 0;
static uint32_t phase2_start = ANIMATION_NORMALIZED_MAX / 3;
static uint32_t phase3_start = 2 * ANIMATION_NORMALIZED_MAX / 3;
static bool animRunning = false;
static AnimationImplementation animImpl;
static Animation *anim;


static void drawRuler(GContext *ctx) {
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

void computeFirstHourAndBitmapPos(int yoffset, int *first_hour, int *bitmap_ypos) {

}

void layer_update(Layer *me, GContext* ctx) {
  int yoffset, bitmap_ypos, y, first_hour, h, pixels_to_move;

  // Compute vertical offset : 60 minutes is 'hour_size' pixels
  yoffset = hour_size * now.tm_min / 60;

  for (first_hour = now.tm_hour, bitmap_ypos = LINE_YPOS - yoffset; bitmap_ypos > 0; first_hour--, bitmap_ypos -= hour_size);

  if (first_hour < 0) {
    first_hour += 24;
  }

  drawRuler(ctx);

  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);

  if (animRunning) {
    // Anim is running, show Date
    // Animated ruler forward to pass midnight then go to the right day number
    // Animation in 3 phases :
    //    - Phase 1 : go to day
    //    - Phase 2 : stay on day
    //    - Phase 3 : go back to hour
    if (anim_time < phase2_start) {
      // Phase 1 : go forward to day, always go through midnight

    } else if (anim_time < phase3_start) {
      // Phase 2 : stay on day

    } else {
      // Phase 3 : go back to hour
    }
  } else {
    // Anim is not running, draw hours
    for (h = first_hour, y = bitmap_ypos; y < 168; h++, y += hour_size) {
      rect.origin.y = y;
      graphics_draw_bitmap_in_rect(ctx, ruler_bitmap, rect);

      if (h >= 24) {
        h -= 24;
      }

      rect_text.origin.y = y - 19;
      snprintf(text, sizeof(text), "%d", h);
      graphics_draw_text(ctx, 	text, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), rect_text, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  }
}

void handleAnim(struct Animation *anim, const uint32_t normTime) {
  anim_time = normTime;
  layer_mark_dirty(layer);

  if (normTime == ANIMATION_NORMALIZED_MAX) {
    animRunning = false;
  }
}

void handle_tap(AccelAxisType axis, int32_t direction) {
  if (!animation_is_scheduled(anim)) {
    animRunning = true;
    animation_schedule(anim);
  }
}

void handle_tick(struct tm *cur, TimeUnits units_changed) {
  now = *cur;

  if (vibration && now.tm_min == 0) {
    vibes_short_pulse();
  }

  layer_mark_dirty(layer);
}

void setColors() {
  if (invert) {
    COLOR_BACKGROUND = GColorBlack;
    COLOR_FOREGROUND = GColorWhite;
  } else {
    COLOR_BACKGROUND = GColorWhite;
    COLOR_FOREGROUND = GColorBlack;
  }
}

void applyConfig() {
  setColors();
  layer_mark_dirty(layer);
}

void logVariables(const char *msg) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MSG: %s\n\tinvert=%d\n\tvibration=%d\n", msg, invert, vibration);
}

bool checkAndSaveInt(int *var, int val, int key) {
  if (*var != val) {
    *var = val;
    persist_write_int(key, val);
    return true;
  } else {
    return false;
  }
}


void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler reason = %d", (int)reason);
}

void in_received_handler(DictionaryIterator *received, void *context) {
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

void readConfig() {
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


void init() {
  time_t t;

  line2_p1 = line1_p1;
  line2_p2 = line1_p2;
  line2_p1.y++;
  line2_p2.y++;

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

  ruler_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RULER);
  // Get bitmap height, this will be the vertical hour span
  hour_size = ruler_bitmap->bounds.size.h;
  rect.size = ruler_bitmap->bounds.size;
  rect_text.origin.x += rect.size.w + 10;

  t = time(NULL);
  now = *(localtime(&t));

  animImpl.setup = NULL;
  animImpl.update = handleAnim;
  animImpl.teardown = NULL;

  anim = animation_create();
  animation_set_delay(anim, 0);
  animation_set_duration(anim, ANIM_DURATION);
  animation_set_implementation(anim, &animImpl);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  accel_tap_service_subscribe(handle_tap);
}


void deinit() {
  if (animRunning) {
    animation_unschedule(anim);
  }
  animation_destroy(anim);
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  gbitmap_destroy(ruler_bitmap);
  layer_destroy(layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
