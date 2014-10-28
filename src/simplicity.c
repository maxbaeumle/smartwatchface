#include "common.h"

Window *window;

TextLayer *text_date_layer;
TextLayer *text_time_layer;

Layer *line_layer;

GBitmap *icon_battery;
Layer *battery_layer;

TextLayer *text_event_title_layer;
TextLayer *text_event_start_date_layer;
TextLayer *text_event_location_layer;

Event event;
BatteryChargeState charge_state;

void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(8, 87), GPoint(131, 87));
  graphics_draw_line(ctx, GPoint(8, 88), GPoint(131, 88));
}

void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
  graphics_draw_bitmap_in_rect(ctx, icon_battery, GRect(35, 0, 24, 12));

  int num = snprintf(NULL, 0, "%u %%", charge_state.charge_percent);
  char *str = malloc(num + 1);
  snprintf(str, num + 1, "%u %%", charge_state.charge_percent);

  graphics_draw_text(ctx, str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(-2, -3, 35-3, 14), 0, GTextAlignmentRight, NULL);

  free(str);

  if (charge_state.charge_percent > 0 && charge_state.charge_percent <= 100) {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(38, 3, (uint8_t)((charge_state.charge_percent * 0.01) * 16), 6), 0, GCornerNone);
  }
}

void handle_battery(BatteryChargeState battery) {
  charge_state = battery;
  layer_mark_dirty(battery_layer);
}

void received_message(DictionaryIterator *received, void *context) {
  Tuple *tuple = dict_find(received, RECONNECT_KEY);

  if (tuple) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (!iter) return;
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    app_message_outbox_send();
  } else {
    Tuple *tuple = dict_find(received, CALENDAR_RESPONSE_KEY);

    if (tuple) {
      if (tuple->length % sizeof(Event) == 1) {
        uint8_t count = tuple->value->data[0];

        if (count == 1) {
          memcpy(&event, &tuple->value->data[1], sizeof(Event));

          text_layer_set_text(text_event_title_layer, event.title);
          text_layer_set_text(text_event_start_date_layer, event.start_date);
          text_layer_set_text(text_event_location_layer, event.has_location ? event.location : "");
        }
      }
    }
  }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);

  if (tick_time->tm_min % 10 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (!iter) return;
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    app_message_outbox_send();
  }
}

void init() {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  text_date_layer = text_layer_create(GRect(8, 94, 144-8, 168-94));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(7, 112, 144-7, 168-112));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_time_layer));

  line_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_get_root_layer(window), line_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  // TODO: Update display here to avoid blank display on launch?

  text_event_title_layer = text_layer_create(GRect(5, 18, layer_get_bounds(window_get_root_layer(window)).size.w - 10, 21));
  text_layer_set_text_color(text_event_title_layer, GColorWhite);
  text_layer_set_background_color(text_event_title_layer, GColorClear);
  text_layer_set_font(text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_title_layer));

  text_event_start_date_layer = text_layer_create(GRect(5, 36, layer_get_bounds(window_get_root_layer(window)).size.w - 10, 21));
  text_layer_set_text_color(text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(text_event_start_date_layer, GColorClear);
  text_layer_set_font(text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_start_date_layer));

  text_event_location_layer = text_layer_create(GRect(5, 54, layer_get_bounds(window_get_root_layer(window)).size.w - 10, 21));
  text_layer_set_text_color(text_event_location_layer, GColorWhite);
  text_layer_set_background_color(text_event_location_layer, GColorClear);
  text_layer_set_font(text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_location_layer));

  icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON);

  GRect frame;
  frame.origin.x = layer_get_bounds(window_get_root_layer(window)).size.w - 66;
  frame.origin.y = 6;
  frame.size.w = 59;
  frame.size.h = 12;

  battery_layer = layer_create(frame);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), battery_layer);

  charge_state = battery_state_service_peek();
  battery_state_service_subscribe(&handle_battery);

  app_message_open(124, 256);
  app_message_register_inbox_received(received_message);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter) return;
  dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
  uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
  dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
  app_message_outbox_send();
}

void deinit() {
  app_message_deregister_callbacks();
  battery_state_service_unsubscribe();
  layer_destroy(battery_layer);
  gbitmap_destroy(icon_battery);
  text_layer_destroy(text_event_location_layer);
  text_layer_destroy(text_event_start_date_layer);
  text_layer_destroy(text_event_title_layer);
  tick_timer_service_unsubscribe();
  layer_destroy(line_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_date_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
