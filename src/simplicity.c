#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "common.h"

#define MY_UUID { 0x69, 0x8B, 0x3E, 0x04, 0xB1, 0x2E, 0x4F, 0xF5, 0xBF, 0xAD, 0x1B, 0xE6, 0xBD, 0xFE, 0xB4, 0xD7 }
PBL_APP_INFO(MY_UUID, "Smartwatch Pro", "Max Baeumle", 1, 0 /* App version */, DEFAULT_MENU_ICON, APP_INFO_WATCH_FACE);

AppContextRef app_context;

Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;

Layer line_layer;

HeapBitmap icon_battery;
Layer battery_layer;

TextLayer text_event_title_layer;
TextLayer text_event_start_date_layer;
TextLayer text_event_location_layer;

Event event;
BatteryStatus battery_status;

void window_unload(Window *window) {
  heap_bitmap_deinit(&icon_battery);
}

void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(8, 87), GPoint(131, 87));
  graphics_draw_line(ctx, GPoint(8, 88), GPoint(131, 88));
}

void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
  graphics_draw_bitmap_in_rect(ctx, &icon_battery.bmp, GRect(0, 0, 24, 12));

  if (battery_status.state != 0 && battery_status.level > 0 && battery_status.level <= 100) {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(3, 3, (uint8_t)((battery_status.level / 100.0) * 16.0), 6), 0, GCornerNone);
  }
}

void handle_init(AppContextRef ctx) {
  app_context = ctx;

  window_init(&window, "Window");
  window_set_window_handlers(&window, (WindowHandlers){
    .unload = window_unload,
  });
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  text_layer_init(&text_date_layer, window.layer.frame);
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  layer_set_frame(&text_date_layer.layer, GRect(8, 94, 144-8, 168-94));
  text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(&window.layer, &text_date_layer.layer);

  text_layer_init(&text_time_layer, window.layer.frame);
  text_layer_set_text_color(&text_time_layer, GColorWhite);
  text_layer_set_background_color(&text_time_layer, GColorClear);
  layer_set_frame(&text_time_layer.layer, GRect(7, 112, 144-7, 168-112));
  text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(&window.layer, &text_time_layer.layer);

  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);

  text_layer_init(&text_event_title_layer, GRect(5, 18, window.layer.bounds.size.w - 10, 21));
  text_layer_set_text_color(&text_event_title_layer, GColorWhite);
  text_layer_set_background_color(&text_event_title_layer, GColorClear);
  text_layer_set_font(&text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_title_layer.layer);

  text_layer_init(&text_event_start_date_layer, GRect(5, 36, window.layer.bounds.size.w - 10, 21));
  text_layer_set_text_color(&text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(&text_event_start_date_layer, GColorClear);
  text_layer_set_font(&text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_start_date_layer.layer);

  text_layer_init(&text_event_location_layer, GRect(5, 54, window.layer.bounds.size.w - 10, 21));
  text_layer_set_text_color(&text_event_location_layer, GColorWhite);
  text_layer_set_background_color(&text_event_location_layer, GColorClear);
  text_layer_set_font(&text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_location_layer.layer);

  heap_bitmap_init(&icon_battery, RESOURCE_ID_BATTERY_ICON);

  GRect frame;
  frame.origin.x = layer_get_bounds(&window.layer).size.w - 31;
  frame.origin.y = 6;
  frame.size.w = 24;
  frame.size.h = 12;

  layer_init(&battery_layer, frame);
  battery_layer.update_proc = &battery_layer_update_callback;
  layer_add_child(&window.layer, &battery_layer);

  battery_status.state = 0;
  battery_status.level = -1;

  // TODO: Update display here to avoid blank display on launch?

  DictionaryIterator *iter;
  app_message_out_get(&iter);
  if (!iter) return;
  dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
  uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
  dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
  app_message_out_send();
  app_message_out_release();

  app_timer_send_event(app_context, 1000, REQUEST_BATTERY_KEY);
}

void received_message(DictionaryIterator *received, void *context) {
  Tuple *tuple = dict_find(received, RECONNECT_KEY);

  if (tuple) {
    DictionaryIterator *iter;
    app_message_out_get(&iter);
    if (!iter) return;
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    app_message_out_send();
    app_message_out_release();

    app_timer_send_event(app_context, 1000, REQUEST_BATTERY_KEY);
  } else {
    Tuple *tuple = dict_find(received, CALENDAR_RESPONSE_KEY);

    if (tuple) {
      if (tuple->length % sizeof(Event) == 1) {
        uint8_t count = tuple->value->data[0];

        if (count == 1) {
          memset(&event, 0, sizeof(Event));
          memcpy(&event, &tuple->value->data[1], sizeof(Event));

          text_layer_set_text(&text_event_title_layer, event.title);
          text_layer_set_text(&text_event_start_date_layer, event.start_date);
          text_layer_set_text(&text_event_location_layer, event.has_location ? event.location : "");
        }
	  }
	} else {
      Tuple *tuple = dict_find(received, BATTERY_RESPONSE_KEY);

      if (tuple) {
        memset(&battery_status, 0, sizeof(BatteryStatus));
        memcpy(&battery_status, &tuple->value->data[0], sizeof(BatteryStatus));

        layer_mark_dirty(&battery_layer);
      }
    }
  }
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  // TODO: Only update the date when it's changed.
  string_format_time(date_text, sizeof(date_text), "%B %e", t->tick_time);
  text_layer_set_text(&text_date_layer, date_text);

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  string_format_time(time_text, sizeof(time_text), time_format, t->tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(&text_time_layer, time_text);

  if (t->tick_time->tm_min % 10 == 0) {
    DictionaryIterator *iter;
    app_message_out_get(&iter);
    if (!iter) return;
    dict_write_int8(iter, REQUEST_CALENDAR_KEY, 0);
    uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
    dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
    app_message_out_send();
    app_message_out_release();

    app_timer_send_event(ctx, 1000, REQUEST_BATTERY_KEY);
  }
}

void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie) {
  app_timer_cancel_event(app_ctx, handle);

  DictionaryIterator *iter;
  app_message_out_get(&iter);

  if (!iter) {
    app_timer_send_event(app_ctx, 1000, cookie);
    return;
  }

  if (cookie == REQUEST_BATTERY_KEY) {
    dict_write_uint8(iter, REQUEST_BATTERY_KEY, 1);
    app_message_out_send();
    app_message_out_release();
  }
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = (PebbleAppMessagingInfo){
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 256,
      },
      .default_callbacks.callbacks = {
        .in_received = received_message,
      },
    },
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    },
    .timer_handler = &handle_timer,
  };
  app_event_loop(params, &handlers);
}
