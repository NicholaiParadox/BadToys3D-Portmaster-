#include "bt3d_app_state.h"
#include "bt3d_debug.h"
#include "bt3d_menu.h"
#include "bt3d_render.h"

#include "profiler.h"

#include "rlgl.h"

#include <stdlib.h>

#if BT3D_PLATFORM_PORTMASTER
/* raylib's built-in OpenGL ES 2 fragment shader declares mediump floats. On
   Mali GPUs that interpolates texture coordinates at half precision, which
   makes textures swim and split. This is the same shader with highp where the
   GPU supports it. Set BT3D_NO_HIGHP_SHADER=1 to compare against the default. */
static const char *const WORLD_VERTEX_SHADER =
    "#version 100\n"
    "attribute vec3 vertexPosition;\n"
    "attribute vec2 vertexTexCoord;\n"
    "attribute vec4 vertexColor;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *const WORLD_FRAGMENT_SHADER =
    "#version 100\n"
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;\n"
    "#else\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "void main() {\n"
    "    vec4 texelColor = texture2D(texture0, fragTexCoord);\n"
    "    gl_FragColor = texelColor*colDiffuse*fragColor;\n"
    "}\n";

static Shader world_shader;
static int world_shader_state; /* 0 = not tried yet, 1 = ready, -1 = unavailable */

static int begin_world_shader(void) {
    if (world_shader_state == 0) {
        world_shader_state = -1;
        if (!getenv("BT3D_NO_HIGHP_SHADER")) {
            world_shader = LoadShaderFromMemory(WORLD_VERTEX_SHADER, WORLD_FRAGMENT_SHADER);
            if (world_shader.id != 0 && world_shader.id != rlGetShaderIdDefault()) {
                world_shader_state = 1;
                TraceLog(LOG_INFO, "BT3D: high precision world shader active");
            } else {
                TraceLog(LOG_WARNING, "BT3D: high precision shader failed, using raylib default");
            }
        }
    }
    if (world_shader_state != 1) return 0;
    BeginShaderMode(world_shader);
    return 1;
}
#endif

static const Color WORLD_CLEAR_COLOR = { 14, 16, 18, 255 };

static void draw_missing_data_pack_screen(void) {
    const Color highlight = { 200, 200, 80, 255 };

    ClearBackground(WORLD_CLEAR_COLOR);
    DrawText("Bad Toys 3D", 40, 80, 36, (Color){ 255, 230, 160, 255 });
    DrawText("data.pck not found.", 40, 140, 22, RAYWHITE);
#if BT3D_PLATFORM_WEB
    DrawText("Reload the page and select the original data.pck.", 40, 180, 20, highlight);
#elif BT3D_PLATFORM_SWITCH
    DrawText("Copy the original data.pck to sdmc:/switch/bt3d/ and restart the game.", 40, 180, 20, highlight);
    DrawText("Press + to quit.", 40, 220, 18, GRAY);
#else
    DrawText("Copy the original data.pck to this folder and restart the game:", 40, 180, 20, highlight);
    DrawText(GetWorkingDirectory(), 40, 212, 18, LIGHTGRAY);
    DrawText("Press Esc to quit.", 40, 252, 18, GRAY);
#endif
}

static void draw_world_scene(AppState *app, Camera3D camera) {
    ClearBackground(WORLD_CLEAR_COLOR);
    BeginMode3D(camera);
#if BT3D_PLATFORM_PORTMASTER
    int world_shader_active = begin_world_shader();
#endif
    BT3D_PROF_BEGIN("draw_world");
    bt3d_draw_map_world(app, camera);
    BT3D_PROF_END("draw_world");
    BT3D_PROF_BEGIN("draw_billboards");
    bt3d_draw_billboard_sprites(app, camera);
    BT3D_PROF_END("draw_billboards");
#if BT3D_PLATFORM_PORTMASTER
    if (world_shader_active) EndShaderMode();
#endif
    EndMode3D();
}

static void draw_death_transition_frame(AppState *app, Camera3D camera) {
    draw_world_scene(app, camera);
    bt3d_draw_real_hud(app);
    bt3d_draw_death_transition(app);
}

static void draw_world_frame(AppState *app, Camera3D camera) {
    draw_world_scene(app, camera);

    bt3d_draw_damage_indicator(app);
    BT3D_PROF_BEGIN("draw_hud");
    bt3d_draw_real_hud(app);
    bt3d_draw_player_weapon_viewmodel(app);
    BT3D_PROF_END("draw_hud");

    if (app->control.noclip_enabled) {
        DrawText("[NOCLIP]", 16, (int)bt3d_hud_rect(app).height + 8, 18, GRAY);
    }
    if (app->settings.debug_tools_enabled) {
        bt3d_draw_debug_overlay(app);
    }
    DrawText("+", GetScreenWidth() / 2 - 6, GetScreenHeight() / 2 - 14, 28, RAYWHITE);
}

static void draw_session_frame(AppState *app, Camera3D camera) {
    if (app->transition.death_active) {
        draw_death_transition_frame(app, camera);
    } else if (app->transition.active) {
        bt3d_draw_level_transition(app);
    } else if (app->control.map_overlay_active) {
        bt3d_draw_original_map_overlay(app);
        bt3d_draw_damage_indicator(app);
    } else {
        draw_world_frame(app, camera);
    }
}

void bt3d_draw_frame(AppState *app, Camera3D camera) {
    BeginDrawing();
    if (app->content.pack.entry_count == 0) {
        draw_missing_data_pack_screen();
    } else if (app->menu.active || !app->session.in_session) {
        bt3d_draw_menu(app);
    } else {
        draw_session_frame(app, camera);
    }
    bt3d_profiler_end_frame();
    EndDrawing();
}
