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

#if defined(PBL_RECT)

#define WIDTH 144
#define HEIGHT 168
#define XOFFSET 0
#define YOFFSET 0

#elif defined(PBL_ROUND)

#define WIDTH 180
#define HEIGHT 180
#define XOFFSET 18
#define YOFFSET 6

#endif

enum {
  CONFIG_KEY_INVERT =       1010,
  CONFIG_KEY_VIBRATION =    1011,
  CONFIG_KEY_LEGACY =       1012,
  CONFIG_KEY_BATTERY =      1013,
  CONFIG_KEY_DATEONSHAKE =  1014,
  CONFIG_KEY_DIAL =         1015,
  CONFIG_KEY_THEMECODE =    1016
};

static Window *window;
static Layer *rootLayer;
static Layer *layer;
static int hour_size = 12 * LINE_INTERVAL; // 12 marks, one every 5 minutes
static int hour_part_size;
static GRect rect_text = { { RULER_XOFFSET + XOFFSET, 0 }, { 60, 60 } };
static GPoint line1_p1 = { 0, 84 };
static GPoint line1_p2 = { 143, 84 };
static GPoint line2_p1;
static GPoint line2_p2;
static GPoint mark1 = { 24+XOFFSET, YOFFSET };
static GPoint mark2 = { XOFFSET, YOFFSET };
static int hour_line_ypos = 84+YOFFSET;

enum {
  COLOR_OUTER_BORDER = 0,
  COLOR_HOUR_DIGIT, // 1
  COLOR_HOUR_MARK, // 2
  COLOR_HALF_MARK, // 3
  COLOR_QUARTER_MARK, // 4
  COLOR_5MIN_MARK, // 5
  COLOR_BATTERY_LEVEL_DIGITS, // 6
  COLOR_BATTERY_MARK_BORDER, // 7
  COLOR_BACKGROUND, // 8
  COLOR_BATTERY_DIAL_BACKGROUND, // 9
  COLOR_BATTERY_MARK_BACKGROUND, // 10
  COLOR_NUM
};

//                               0 1 2 3 4 5 6 7 8 9 10
static char themeCodeText[30] = "d0e0e0d0c0c0c0e0fdfdf4";
static GColor colorTheme[COLOR_NUM];


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
static int dial = true;

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

static void drawBackground(GContext *ctx) {
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, colorTheme[COLOR_BACKGROUND]);
#else
  graphics_context_set_fill_color(ctx, bgColor);
#endif
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void drawDial(GContext *ctx) {
  static GRect left   = { {   0,   0 }, {   9, 168 } };
  static GRect right  = { { 135,   0 }, {   9, 168 } };
  static GRect top    = { {   0,   0 }, { 144,   5 } };
  GRect bottom = { { 0, 2*hour_line_ypos-4 }, { 144, 172-2*hour_line_ypos } };
  GRect line  = { { 0, hour_line_ypos-1 }, { WIDTH, 2 } };
  GRect lineleft  = { { 0, hour_line_ypos-5 }, { 4, 10 } };
  GRect lineright = { { WIDTH-4, hour_line_ypos-5 }, { 4, 10 } };
  
  if (dial) {
#if defined(PBL_RECT)
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, colorTheme[COLOR_OUTER_BORDER]);
    graphics_context_set_stroke_color(ctx, colorTheme[COLOR_OUTER_BORDER]);
  #else
    graphics_context_set_fill_color(ctx, fgColor);
    graphics_context_set_stroke_color(ctx, fgColor);
  #endif
    graphics_fill_rect(ctx, left, 0, GCornerNone);
    graphics_fill_rect(ctx, right, 0, GCornerNone);
    graphics_fill_rect(ctx, top, 0, GCornerNone);
    graphics_fill_rect(ctx, bottom, 0, GCornerNone);
    graphics_fill_rect(ctx, line, 0, GCornerNone);

  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, colorTheme[COLOR_BACKGROUND]);
  #else
    graphics_context_set_fill_color(ctx, bgColor);
  #endif

    graphics_fill_rect(ctx, GRect(5, 5 + YOFFSET, 5, hour_line_ypos-6), 4, GCornersLeft);
    graphics_fill_rect(ctx, GRect(WIDTH-10, 5 + YOFFSET, 5, hour_line_ypos-6), 4, GCornersRight);
    graphics_fill_rect(ctx, GRect(5, hour_line_ypos+1 + YOFFSET, 5, hour_line_ypos-5), 4, GCornersLeft);
    graphics_fill_rect(ctx, GRect(WIDTH-10, hour_line_ypos+1 + YOFFSET, 5, hour_line_ypos-5), 4, GCornersRight);
#elif defined(PBL_ROUND)

    graphics_context_set_fill_color(ctx, colorTheme[COLOR_OUTER_BORDER]);
    graphics_context_set_stroke_color(ctx, colorTheme[COLOR_OUTER_BORDER]);
    graphics_context_set_stroke_width(ctx, 10);

    graphics_draw_arc(ctx, GRect (0, 0, WIDTH, HEIGHT), GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);
    graphics_fill_rect(ctx, line, 0, GCornerNone);
    graphics_context_set_stroke_width(ctx, 1);

#endif
  } else {
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, colorTheme[COLOR_OUTER_BORDER]);
  #else
    graphics_context_set_fill_color(ctx, fgColor);
  #endif
    graphics_fill_rect(ctx, line, 0, GCornerNone);
    graphics_fill_rect(ctx, lineleft, 0, GCornerNone);
    graphics_fill_rect(ctx, lineright, 0, GCornerNone);    

  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, colorTheme[COLOR_BACKGROUND]);
  #else
    graphics_context_set_fill_color(ctx, bgColor);
  #endif
    graphics_fill_rect(ctx, GRect(0, hour_line_ypos-6, 5, 5), 4, GCornerBottomLeft);
    graphics_fill_rect(ctx, GRect(WIDTH-5, hour_line_ypos-6, 5, 5), 4, GCornerBottomRight);
    graphics_fill_rect(ctx, GRect(0, hour_line_ypos+1, 5, 5), 4, GCornerTopLeft);
    graphics_fill_rect(ctx, GRect(WIDTH-5, hour_line_ypos+1, 5, 5), 4, GCornerTopRight);
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
      if ((y > -20) && (y < HEIGHT+20)) {
        if ((y >= 0) && (y < HEIGHT)) {
          mark1.y = mark2.y = y;
          mark2.x = mark1.x + markWidth[j];
        #ifdef PBL_COLOR
          if (j == 0) {
            graphics_context_set_stroke_color(ctx, colorTheme[COLOR_HOUR_MARK]);
          } else if (j == 6) {
            graphics_context_set_stroke_color(ctx, colorTheme[COLOR_HALF_MARK]);
          } else if ((j == 3) || (j == 9)) {
            graphics_context_set_stroke_color(ctx, colorTheme[COLOR_QUARTER_MARK]);
          } else {
            graphics_context_set_stroke_color(ctx, colorTheme[COLOR_5MIN_MARK]);
          }
        #endif
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
        #ifdef PBL_COLOR
          graphics_context_set_text_color(ctx, colorTheme[COLOR_HOUR_DIGIT]);
        #else
          graphics_context_set_text_color(ctx, fgColor);
        #endif
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
  static int batteryGaugeYOffset = 2*YOFFSET;
  static GPoint battery_mark_pos = {0, 0};
#if defined(PBL_ROUND)
  static int32_t angle1 = 3*TRIG_MAX_ANGLE/8;
  static int32_t angle2 = 5*TRIG_MAX_ANGLE/8;
  static int32_t a = 0;
  static int32_t r = 0;
  static int32_t s = TRIG_MAX_ANGLE/40;
  static int32_t cos, sin;
  static GRect rect1 = { {18, 18}, {144, 144} };
  static GRect rect2 = { {12, 12}, {156, 156} };
  static GPoint p1 = { 0, 0 };
  static GPoint p2 = { 0, 0 };
#endif


#if defined(PBL_RECT)
  battery_mark_pos = GPoint(7+((int)battery_level*12/10) + XOFFSET, 144 - batteryGaugeYOffset);

  // Battery Gauge Background
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, colorTheme[COLOR_BATTERY_DIAL_BACKGROUND]);
#else
  graphics_context_set_fill_color(ctx, bgColor);
#endif
  graphics_fill_rect(ctx, GRect(4 + XOFFSET, 148 - batteryGaugeYOffset, 136, 16), 4, GCornersAll);

#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_MARK_BORDER]);
#else
  graphics_context_set_stroke_color(ctx, fgColor);
#endif
  graphics_draw_round_rect(ctx, GRect(4 + XOFFSET, 148 - batteryGaugeYOffset, 136, 16), 4);

  // Battery line and graduations
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_LEVEL_DIGITS]);
  graphics_context_set_text_color(ctx, colorTheme[COLOR_BATTERY_LEVEL_DIGITS]);
#else
  graphics_context_set_stroke_color(ctx, fgColor);
  graphics_context_set_text_color(ctx, fgColor);
#endif
  graphics_draw_line(ctx, GPoint(11 + XOFFSET, 161 - batteryGaugeYOffset), GPoint(131 + XOFFSET, 161 - batteryGaugeYOffset));
  for (i=0, x=11 + XOFFSET; i<=10; i++, x+=12) {
    graphics_draw_line(ctx, GPoint(x, 159 - batteryGaugeYOffset), GPoint(x, 161 - batteryGaugeYOffset));
    graphics_draw_text(ctx, t[i], battery_gauge_font, GRect(x-9, 150 - batteryGaugeYOffset, 20, 11), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  // Battery mark
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_MARK_BORDER]);
  graphics_context_set_fill_color(ctx, colorTheme[COLOR_BATTERY_MARK_BACKGROUND]);
#else
  graphics_context_set_stroke_color(ctx, fgColor);
  graphics_context_set_fill_color(ctx, bgColor);
#endif
  gpath_move_to(battery_mark_upper_path, battery_mark_pos);
  gpath_move_to(battery_mark_lower_path, battery_mark_pos);
  gpath_draw_filled(ctx, battery_mark_upper_path);
  gpath_draw_filled(ctx, battery_mark_lower_path);
  gpath_draw_outline(ctx, battery_mark_upper_path);
  gpath_draw_outline(ctx, battery_mark_lower_path);

#elif defined(PBL_ROUND)

  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_MARK_BORDER]);
  graphics_context_set_stroke_width(ctx, 18);
  graphics_draw_arc	(ctx, rect1, GOvalScaleModeFitCircle, angle1, angle2);

  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_DIAL_BACKGROUND]);
  graphics_context_set_stroke_width(ctx, 16);
  graphics_draw_arc	(ctx, rect1, GOvalScaleModeFitCircle, angle1, angle2);

  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_LEVEL_DIGITS]);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_arc	(ctx, rect2, GOvalScaleModeFitCircle, angle1, angle2);

  graphics_context_set_text_color(ctx, colorTheme[COLOR_BATTERY_LEVEL_DIGITS]);

  for (i=0, a=angle1; i<=10; i++, a-=s) {
    int32_t cos = cos_lookup(a);
    int32_t sin = sin_lookup(a);

    p1.x = 90 + 75*cos/TRIG_MAX_RATIO;
    p1.y = 90 + 75*sin/TRIG_MAX_RATIO;
    p2.x = 90 + 78*cos/TRIG_MAX_RATIO;
    p2.y = 90 + 78*sin/TRIG_MAX_RATIO;

    graphics_draw_line(ctx, p1, p2);
    graphics_draw_text(ctx, t[i], battery_gauge_font, GRect(p1.x-(i<8?8:10), p1.y-(i==5?9:10), 20, 11), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  graphics_context_set_stroke_color(ctx, colorTheme[COLOR_BATTERY_MARK_BORDER]);
  graphics_context_set_fill_color(ctx, colorTheme[COLOR_BATTERY_MARK_BACKGROUND]);

  a = angle1-battery_level*s/10;
  r = 59 + battery_level/25;
  battery_mark_pos.x = 86 + r * cos_lookup(a)/TRIG_MAX_RATIO;
  battery_mark_pos.y = 90 + r * sin_lookup(a)/TRIG_MAX_RATIO;

  gpath_rotate_to(battery_mark_upper_path, a - TRIG_MAX_ANGLE/4);
  gpath_rotate_to(battery_mark_lower_path, a - TRIG_MAX_ANGLE/4);
  gpath_move_to(battery_mark_upper_path, battery_mark_pos);
  gpath_move_to(battery_mark_lower_path, battery_mark_pos);
  gpath_draw_filled(ctx, battery_mark_upper_path);
  gpath_draw_filled(ctx, battery_mark_lower_path);
  gpath_draw_outline(ctx, battery_mark_upper_path);
  gpath_draw_outline(ctx, battery_mark_lower_path);
#endif
}

static void layer_update(Layer *me, GContext* ctx) {
  drawBackground(ctx);
  drawRuler(ctx);
  drawDial(ctx);
  if (battery) {
    drawBattery(ctx);
  }
}

static void createAnim();
static void destroyAnim();

static void rescheduleAnim(Animation *anim, bool finished, void *ctx) {
  destroyAnim();

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "-> rescheduleAnim: phase=%d - animRunning = %d", anim_phase, animRunning);

  anim_phase++;
  if (anim_phase == 2) {
    anim_time = ANIMATION_NORMALIZED_MIN;
    createAnim();
    animation_schedule(anim);
  } else {
    anim_phase = 0;
    animRunning = false;
  }

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "<- rescheduleAnim: phase=%d - animRunning = %d", anim_phase, animRunning);
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

static int hexCharToInt(const char digit) {
  if ((digit >= '0') && (digit <= '9')) {
    return (int)(digit - '0');
  } else if ((digit >= 'a') && (digit <= 'f')) {
    return 10 + (int)(digit - 'a');
  } else if ((digit >= 'A') && (digit <= 'F')) {
    return 10 + (int)(digit - 'A');
  } else {
    return -1;
  }
}

static int hexStringToByte(const char *hexString) {
  int l = strlen(hexString);
  if (l == 0) return 0;
  if (l == 1) return hexCharToInt(hexString[0]);
  
  return 16*hexCharToInt(hexString[0]) + hexCharToInt(hexString[1]);
}

void decodeThemeCode(char *code) {
#ifdef PBL_COLOR
  int i;
  
  for (i=0; i<11; i++) {
    colorTheme[i] = (GColor8){.argb=(uint8_t)hexStringToByte(code + 2*i)};
  }
#else
  // Do nothing on APLITE
#endif
}

static void setColors() {
  if (invert) {
    bgColor = GColorBlack;
    fgColor = GColorWhite;
  } else {
    bgColor = GColorWhite;
    fgColor = GColorBlack;
  }
  
#ifdef PBL_COLOR
  APP_LOG(APP_LOG_LEVEL_DEBUG, "decodeThemeCode from setColors (%s)", themeCodeText);
  decodeThemeCode(themeCodeText);
#endif
}

static void setHourLinePoints() {
  hour_line_ypos = HEIGHT / 2;

#if defined(PBL_RECT)
  if (battery) {
    hour_line_ypos -= 11;
  }
#endif
  
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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MSG: %s\n\tinvert=%d\n\tvibration=%d\n\tlegacy=%d\n\tbattery=%d\n\tdateonshake=%d\n\tdial=%d\n",
    msg, invert, vibration, legacy, battery, dateonshake, dial);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "\tthemecode=%s\n", themeCodeText);
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

bool checkAndSaveString(char *var, char *val, int key) {
  int ret;

  if (strcmp(var, val) != 0) {
    strcpy(var, val);
    ret = persist_write_string(key, val);
    if (ret < (int)strlen(val)) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "checkAndSaveString() : persist_write_string(%d, %s) returned %d",
              key, val, ret);
    }
    return true;
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "checkAndSaveString() : value has not changed (was : %s, is : %s)",
        var, val);
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
  Tuple *dialTuple = dict_find(received, CONFIG_KEY_DIAL);
  Tuple *themeCodeTuple = dict_find(received, CONFIG_KEY_THEMECODE);

  if (invertTuple && vibrationTuple && legacyTuple && batteryTuple && dateonshakeTuple && dialTuple && themeCodeTuple) {
    somethingChanged |= checkAndSaveInt(&invert, invertTuple->value->int32, CONFIG_KEY_INVERT);
    somethingChanged |= checkAndSaveInt(&vibration, vibrationTuple->value->int32, CONFIG_KEY_VIBRATION);
    somethingChanged |= checkAndSaveInt(&legacy, legacyTuple->value->int32, CONFIG_KEY_LEGACY);
    somethingChanged |= checkAndSaveInt(&battery, batteryTuple->value->int32, CONFIG_KEY_BATTERY);
    somethingChanged |= checkAndSaveInt(&dateonshake, dateonshakeTuple->value->int32, CONFIG_KEY_DATEONSHAKE);
    somethingChanged |= checkAndSaveInt(&dial, dialTuple->value->int32, CONFIG_KEY_DIAL);
    somethingChanged |= checkAndSaveString(themeCodeText, themeCodeTuple->value->cstring, CONFIG_KEY_THEMECODE);
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

  if (persist_exists(CONFIG_KEY_DIAL)) {
    dial = persist_read_int(CONFIG_KEY_DIAL);
  } else {
    dial = 1;
  }

  if (persist_exists(CONFIG_KEY_THEMECODE)) {
    persist_read_string(CONFIG_KEY_THEMECODE, themeCodeText, sizeof(themeCodeText));
  } else {
    strcpy(themeCodeText, "d0e0e0d0c0c0c0e0fdfdf4");
    persist_write_string(CONFIG_KEY_THEMECODE, themeCodeText);
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "decodeThemeCode from readConfig (%s)", themeCodeText);
  decodeThemeCode(themeCodeText);

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

  setColors();

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

  layer = layer_create(layer_get_bounds(rootLayer));
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
