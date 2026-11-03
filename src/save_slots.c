#include "bt3d_app_state.h"
#include "bt3d_enemy.h"
#include "bt3d_platform.h"
#include "bt3d_save_internal.h"
#include "bt3d_save_slots.h"
#include "bt3d_savegame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Slots live in save-slot-1.dat .. save-slot-5.dat. */
static int slot_path(int slot, char *out_path, size_t out_path_size) {
    char file_name[32];

    if (slot < 0 || slot >= BT3D_SAVE_SLOT_COUNT) return 0;
    snprintf(file_name, sizeof(file_name), "save-slot-%d.dat", slot + 1);
    return bt3d_build_save_path(file_name, out_path, out_path_size);
}

int bt3d_save_game_to_latest_slot(AppState *app) {
    char path[256];
    return bt3d_build_quicksave_path(path, sizeof(path)) && bt3d_save_game_to_file(app, path);
}

int bt3d_load_game_from_latest_slot(AppState *app) {
    char path[256];
    return bt3d_build_quicksave_path(path, sizeof(path)) && load_game_from_file(app, path);
}

/* The five slots first (empty ones included), then the autosave if there is one.
   Labels carry just the level, e.g. "Slot 2 - Level 4". */
void refresh_save_entries(AppState *app) {
    char path[256];
    SaveHeader header;
    int slot;

    app->saves.count = 0;
    for (slot = 0; slot < BT3D_SAVE_SLOT_COUNT; ++slot) {
        SaveEntry *entry = &app->saves.entries[app->saves.count++];

        entry->path[0] = '\0';
        entry->mod_time = 0;
        entry->occupied = 0;
        if (slot_path(slot, path, sizeof(path))) {
            snprintf(entry->path, sizeof(entry->path), "%s", path);
            entry->occupied = bt3d_read_save_header(path, &header);
        }
        if (entry->occupied) {
            snprintf(entry->label, sizeof(entry->label), "Slot %d - Level %d", slot + 1, header.map_index + 1);
        } else {
            snprintf(entry->label, sizeof(entry->label), "Slot %d - Empty", slot + 1);
        }
    }

    if (bt3d_build_quicksave_path(path, sizeof(path)) && bt3d_read_save_header(path, &header)) {
        SaveEntry *entry = &app->saves.entries[app->saves.count++];

        snprintf(entry->path, sizeof(entry->path), "%s", path);
        snprintf(entry->label, sizeof(entry->label), "Autosave - Level %d", header.map_index + 1);
        entry->mod_time = 0;
        entry->occupied = 1;
    }
}

/* Writes the current game into slot 0..4, replacing whatever was there. */
int save_game_to_slot(AppState *app, int slot) {
    char path[256];

    if (!slot_path(slot, path, sizeof(path))) return 0;
    if (!bt3d_save_game_to_file(app, path)) return 0;
    refresh_save_entries(app);
    return 1;
}

int delete_save_file(AppState *app, const char *path) {
    if (remove(path) != 0) return 0;
    bt3d_sync_persistent_storage();
    refresh_save_entries(app);
    return 1;
}
