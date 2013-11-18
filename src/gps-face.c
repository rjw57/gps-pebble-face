#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *latitude_text_layer;
static TextLayer *longitude_text_layer;

static uint8_t* loc_sync_buffer;
static AppSync loc_sync;

#define KEY_LATITUDE    0
#define KEY_LONGITUDE   1
#define KEY_ACCURACY    2
#define KEY_HEADING     3
#define KEY_SPEED       4
#define KEY_BNG         5
#define KEY_REQUEST_LOC 6

struct {
    const char* latitude;
    const char* longitude;
    const char* bng_reference;
    time_t last_response;
} state;

static void send_cmd(void) {
    Tuplet value = TupletInteger(KEY_REQUEST_LOC, 1);

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    if (iter == NULL) {
        return;
    }

    dict_write_tuplet(iter, &value);
    dict_write_end(iter);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending ping to app");
    app_message_outbox_send();
}

static void refresh_face() {
    text_layer_set_text(text_layer, state.bng_reference);
    text_layer_set_text(latitude_text_layer, state.latitude);
    text_layer_set_text(longitude_text_layer, state.longitude);
}

/* Called to update face */
static void handle_tick(struct tm* tick_time, TimeUnits units_changed) {
    if((0 == strcmp(state.bng_reference, "")) || (state.bng_reference[0] == ' ') || (state.last_response + 15 < time(NULL))) {
        send_cmd();
    }
}

/* Called when some part of the location dictionary has changed. */
static void handle_loc_tuple_changed(
        const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context)
{
    switch(key) {
        case KEY_LATITUDE:
            state.latitude = new_tuple->value->cstring;
            state.last_response = time(NULL);
            refresh_face();
            break;
        case KEY_LONGITUDE:
            state.longitude = new_tuple->value->cstring;
            state.last_response = time(NULL);
            refresh_face();
            break;
        /*
        case KEY_ACCURACY:
            state.accuracy = (float)(new_tuple->value->int32) / (1<<16);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Got accuracy = %f", state.accuracy);
            break;
            */
        case KEY_BNG:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Got reference = >%s<", new_tuple->value->cstring);
            state.bng_reference = new_tuple->value->cstring;
            state.last_response = time(NULL);
            refresh_face();
            break;
    }
}

/* Called when there is an error with an AppMessage updating the location dictionary. */
static void handle_loc_tuple_error(
        DictionaryResult dict_error, AppMessageResult app_message_error, void *context)
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "App Message Sync Error: %d", app_message_error);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    text_layer = text_layer_create((GRect) { .origin = { 0, 52 }, .size = { bounds.size.w, 20 } });
    text_layer_set_text(text_layer, "Press a button");
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);

    latitude_text_layer = text_layer_create((GRect) { .origin = { 0, 74 }, .size = { bounds.size.w, 20 } });
    text_layer_set_text(latitude_text_layer, "Press a button");
    text_layer_set_font(latitude_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(latitude_text_layer, GTextAlignmentCenter);

    longitude_text_layer = text_layer_create((GRect) { .origin = { 0, 96 }, .size = { bounds.size.w, 20 } });
    text_layer_set_text(longitude_text_layer, "Press a button");
    text_layer_set_font(longitude_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(longitude_text_layer, GTextAlignmentCenter);

    state.bng_reference = "";
    state.latitude = "";
    state.longitude = "";
    state.last_response = 0;

    /* Initialise AppSync service */
    const Tuplet initial_location_values[] = {
        /* TupletInteger(KEY_LATITUDE, 0),
        TupletInteger(KEY_LONGITUDE, 0),
        TupletInteger(KEY_ACCURACY, 0),
        TupletInteger(KEY_HEADING, 0),
        TupletInteger(KEY_SPEED, 0), */
        TupletCString(KEY_LATITUDE, "xxx\u00b0xx'xx\"X"),
        TupletCString(KEY_LONGITUDE, "xxx\u00b0xx'xx\"X"),
        TupletCString(KEY_BNG, "            "),
    };

    const int initial_location_value_count =
        sizeof(initial_location_values)/sizeof(initial_location_values[0]);

    uint16_t loc_sync_buffer_size = dict_calc_buffer_size_from_tuplets(
                initial_location_values, initial_location_value_count);

    const int inbound_size = 124;
    const int outbound_size = 124;
    app_message_open(inbound_size, outbound_size);

    loc_sync_buffer = malloc(loc_sync_buffer_size);
    app_sync_init(&loc_sync, loc_sync_buffer, loc_sync_buffer_size,
            initial_location_values, initial_location_value_count,
            &handle_loc_tuple_changed, &handle_loc_tuple_error,
            NULL);

    // Ensures time is displayed immediately (will break if NULL tick event accessed).
    // (This is why it's a good idea to have a separate routine to do the update itself.)
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    handle_tick(current_time, SECOND_UNIT);
    tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);

    layer_add_child(window_layer, text_layer_get_layer(text_layer));
    layer_add_child(window_layer, text_layer_get_layer(latitude_text_layer));
    layer_add_child(window_layer, text_layer_get_layer(longitude_text_layer));

    // Ping the phone app to send us the current location
    send_cmd();
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
    text_layer_destroy(latitude_text_layer);
    text_layer_destroy(longitude_text_layer);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
            });
    window_stack_push(window, true);
}

static void deinit(void) {
    window_destroy(window);
    app_sync_deinit(&loc_sync);
}

int main(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialising app");
    init();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialisation complete");
    app_event_loop();
    deinit();
}
