/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file thing_navigate.h
 *     Header file for thing_navigate.c.
 * @par Purpose:
 *     Things movement navigation functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     25 Mar 2009 - 12 Aug 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_TNGNAVIGATE_H
#define DK_TNGNAVIGATE_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

enum SlabBlockedFlags {
    SlbBloF_None    = 0x00,
    SlbBloF_WalledX = 0x01,
    SlbBloF_WalledY = 0x02,
    SlbBloF_WalledZ = 0x04,
};

/******************************************************************************/
#pragma pack(1)

struct Thing;
struct Room;

/******************************************************************************/
DLLIMPORT long _DK_owner_player_navigating;
#define owner_player_navigating _DK_owner_player_navigating
DLLIMPORT long _DK_nav_thing_can_travel_over_lava;
#define nav_thing_can_travel_over_lava _DK_nav_thing_can_travel_over_lava

#pragma pack()
/******************************************************************************/
TbBool setup_person_move_to_position(struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned char storage);
TbBool setup_person_move_close_to_position(struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned char storage);
TbBool setup_person_move_backwards_to_position(struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned char storage);
TbBool setup_person_move_to_coord(struct Thing *thing, struct Coord3d *pos, unsigned char storage);
TbBool setup_person_move_backwards_to_coord(struct Thing *thing, struct Coord3d *pos, unsigned char storage);
TbBool person_move_somewhere_adjacent_in_room(struct Thing *thing, const struct Room *room);

TbBool creature_can_travel_over_lava(const struct Thing *thing);
TbBool creature_can_navigate_to_f(struct Thing *thing, struct Coord3d *pos, TbBool no_owner, const char *func_name);
#define creature_can_navigate_to(thing,pos,no_owner) creature_can_navigate_to_f(thing,pos,no_owner,__func__)
TbBool creature_can_navigate_to_with_storage_f(struct Thing *crtng, struct Coord3d *pos, unsigned char storage, const char *func_name);
#define creature_can_navigate_to_with_storage(crtng,pos,storage) creature_can_navigate_to_with_storage_f(crtng,pos,storage,__func__)
TbBool creature_can_get_to_dungeon(struct Thing *thing, PlayerNumber plyr_idx);
struct Thing *find_hero_door_hero_can_navigate_to(struct Thing *herotng);
unsigned char get_nearest_valid_position_for_creature_at(struct Thing *thing, struct Coord3d *pos);

long creature_move_to_using_gates(struct Thing *thing, struct Coord3d *pos, MoveSpeed speed, long a4, long a5, TbBool backward);
long creature_move_to(struct Thing *thing, struct Coord3d *pos, MoveSpeed speed, unsigned char a4, TbBool backward);
void move_thing_in_map(struct Thing *thing, const struct Coord3d *pos);
short move_to_position(struct Thing *thing);
long creature_turn_to_face(struct Thing *thing, struct Coord3d *pos);
long creature_turn_to_face_backwards(struct Thing *thing, struct Coord3d *pos);
long creature_turn_to_face_angle(struct Thing *thing, long a2);
TbBool move_creature_to_nearest_valid_position(struct Thing *thing);
long get_next_gap_creature_can_fit_in_below_point(struct Thing *thing, struct Coord3d *pos);
long thing_covers_same_blocks_in_two_positions(struct Thing *thing, struct Coord3d *pos1, struct Coord3d *pos2);
long get_thing_blocked_flags_at(struct Thing *thing, struct Coord3d *pos);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
