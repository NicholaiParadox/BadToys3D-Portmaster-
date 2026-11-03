#include "bt3d_app_state.h"
#include "bt3d_math.h"
#include "bt3d_render.h"
#include "menu_internal.h"

#include <math.h>

static Color button_text_color(int selected) {
    return selected ? (Color){ 28, 22, 8, 255 } : (Color){ 255, 238, 160, 255 };
}

/* Largest font size (up to max_size) at which `text` fits in max_width. */
static int fit_font_size(const char *text, float max_width, int max_size, int min_size) {
    int size = max_size;
    while (size > min_size && (float)MeasureText(text, size) > max_width) size--;
    return size;
}

/* Text is sized from the button's height and shrunk to fit its width, so it
   never spills out of the box on small screens. */
static void draw_button(Rectangle rect, const char *label, int selected, float text_width) {
    Color fill = selected ? (Color){ 232, 184, 48, 235 } : (Color){ 174, 124, 30, 220 };
    Color border = selected ? (Color){ 255, 230, 92, 255 } : (Color){ 210, 164, 52, 230 };
    int max_size = (int)fminf(22.0f, rect.height * 0.62f);
    int size = fit_font_size(label, text_width, max_size < 10 ? 10 : max_size, 9);

    DrawRectangleRounded(rect, 0.08f, 6, fill);
    DrawRectangleLinesEx(rect, 2.0f, border);
    DrawText(label, (int)rect.x + 12, (int)(rect.y + (rect.height - (float)size) * 0.5f), size, button_text_color(selected));
}

static void draw_slider(Rectangle item, float value, int selected) {
    Rectangle slider = menu_slider_rect(item);
    float knob_x = slider.x + slider.width * bt3d_clamp01(value);

    DrawRectangleRounded(slider, 0.6f, 6, (Color){ 64, 45, 20, 210 });
    DrawRectangleRounded((Rectangle){ slider.x, slider.y, knob_x - slider.x, slider.height }, 0.6f, 6, (Color){ 255, 230, 92, 255 });
    DrawCircle((int)knob_x, (int)(slider.y + slider.height * 0.5f), 8.0f, button_text_color(selected));
}

void bt3d_draw_menu(const AppState *app) {
    MenuPage page;
    int i;

    menu_build_page(app, &page);
    if (!bt3d_draw_pod_background(app)) ClearBackground((Color){ 10, 8, 7, 255 });
    {
        float ui = fminf(1.0f, fmaxf(0.6f, (float)GetScreenHeight() / 720.0f));
        DrawText(page.title, (int)page.title_position.x, (int)page.title_position.y, (int)(26.0f * ui), RAYWHITE);
    }

    for (i = 0; i < page.count; ++i) {
        const MenuItem *item = &page.items[i];
        int selected = i == app->menu.index;

        if (item->rect.width <= 0.0f) continue;
        {
            float text_width = item->rect.width - 24.0f;
            if (item->action == MENU_ITEM_MUSIC || item->action == MENU_ITEM_SOUND) {
                /* Leave the slider's side of the button free. */
                text_width = menu_slider_rect(item->rect).x - item->rect.x - 24.0f;
            }
            draw_button(item->rect, item->label, selected, text_width);
        }
        if (item->action == MENU_ITEM_MUSIC) draw_slider(item->rect, app->settings.music_volume, selected);
        if (item->action == MENU_ITEM_SOUND) draw_slider(item->rect, app->settings.sound_volume, selected);
    }
    if (page.hint) {
        float ui = fminf(1.0f, fmaxf(0.6f, (float)GetScreenHeight() / 720.0f));
        int size = fit_font_size(page.hint, page.panel.width - 40.0f, (int)(18.0f * ui), 9);
        DrawText(page.hint, (int)page.panel.x + 20, (int)(page.panel.y + page.panel.height - (float)size - 10.0f), size, LIGHTGRAY);
    }
}
