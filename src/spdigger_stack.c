/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file spdigger_stack.c
 *     Special diggers task stack support functions.
 * @par Purpose:
 *     Functions to create and maintain list of tasks for special diggers (imps).
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     11 Mar 2010 - 04 May 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "spdigger_stack.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_math.h"

#include "creature_states.h"
#include "creature_states_train.h"
#include "map_blocks.h"
#include "dungeon_data.h"
#include "tasks_list.h"
#include "config_creature.h"
#include "thing_corpses.h"
#include "thing_navigate.h"
#include "thing_stats.h"
#include "room_data.h"
#include "thing_objects.h"
#include "map_events.h"
#include "gui_soundmsgs.h"

#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT long _DK_imp_stack_update(struct Thing *thing);
DLLIMPORT long _DK_add_unclaimed_unconscious_bodies_to_imp_stack(struct Dungeon *dungeon, long a2);
DLLIMPORT long _DK_add_unclaimed_dead_bodies_to_imp_stack(struct Dungeon *dungeon, long a2);
DLLIMPORT long _DK_add_unclaimed_spells_to_imp_stack(struct Dungeon *dungeon, long a2);
DLLIMPORT void _DK_add_pretty_and_convert_to_imp_stack(struct Dungeon *dungeon);
DLLIMPORT long _DK_add_unclaimed_gold_to_imp_stack(struct Dungeon *dungeon);
DLLIMPORT long _DK_add_object_for_trap_to_imp_stack(struct Dungeon *dungeon, struct Thing *thing);
DLLIMPORT long _DK_check_out_imp_stack(struct Thing *thing);
DLLIMPORT struct Thing *_DK_check_for_empty_trap_for_imp_not_being_armed(struct Thing *thing, long a2);
DLLIMPORT long _DK_imp_will_soon_be_working_at_excluding(struct Thing *thing, long a2, long a3);
DLLIMPORT long _DK_check_out_imp_last_did(struct Thing *thing);
DLLIMPORT long _DK_check_place_to_convert_excluding(struct Thing *thing, long a2, long a3);
DLLIMPORT long _DK_check_out_unconverted_spiral(struct Thing *thing, long a2);
DLLIMPORT long _DK_check_place_to_pretty_excluding(struct Thing *thing, long a2, long a3);
DLLIMPORT long _DK_check_out_unprettied_spiral(struct Thing *thing, long a2);
DLLIMPORT long _DK_check_out_undug_place(struct Thing *thing);
DLLIMPORT long _DK_check_out_undug_area(struct Thing *thing);
DLLIMPORT long _DK_check_out_unprettied_or_unconverted_area(struct Thing *thing);
DLLIMPORT long _DK_check_out_unreinforced_place(struct Thing *thing);
DLLIMPORT long _DK_check_out_unreinforced_area(struct Thing *thing);
DLLIMPORT long _DK_check_out_uncrowded_reinforce_position(struct Thing *thing, unsigned short a2, long *a3, long *a4);
DLLIMPORT long _DK_check_place_to_dig_and_get_position(struct Thing *thing, unsigned short a2, long *a3, long *a4);
DLLIMPORT struct Thing *_DK_check_place_to_pickup_dead_body(struct Thing *thing, long stl_x, long stl_y);
DLLIMPORT struct Thing *_DK_check_place_to_pickup_gold(struct Thing *thing, long stl_x, long stl_y);
DLLIMPORT struct Thing *_DK_check_place_to_pickup_spell(struct Thing *thing, long a2, long a3);
DLLIMPORT struct Thing *_DK_check_place_to_pickup_unconscious_body(struct Thing *thing, long a2, long a3);
DLLIMPORT long _DK_check_place_to_reinforce(struct Thing *thing, long a2, long a3);
DLLIMPORT struct Thing *_DK_check_place_to_pickup_crate(struct Thing *thing, long stl_x, long stl_y);
/******************************************************************************/
long const dig_pos[] = {0, -1, 1};

/******************************************************************************/
TbBool add_to_imp_stack_using_pos(long stl_num, long task_type, struct Dungeon *dungeon)
{
    struct DiggerStack *istack;
    if (dungeon->digger_stack_length >= IMP_TASK_MAX_COUNT)
        return false;
    istack = &dungeon->imp_stack[dungeon->digger_stack_length];
    dungeon->digger_stack_length++;
    istack->field_0 = stl_num;
    istack->task_id = task_type;
    return (dungeon->digger_stack_length < IMP_TASK_MAX_COUNT);
}

long imp_will_soon_be_working_at_excluding(struct Thing *thing, long a2, long a3)
{
    SYNCDBG(19,"Starting");
    return _DK_imp_will_soon_be_working_at_excluding(thing, a2, a3);
}

/** Returns if the player owns any digger who is working on re-arming it.
 *
 * @param traptng The trap that needs re-arming.
 * @return
 */
TbBool imp_will_soon_be_arming_trap(struct Thing *traptng)
{
    struct Dungeon *dungeon;
    struct Thing *thing;
    struct CreatureControl *cctrl;
    long crstate;
    long i;
    unsigned long k;
    //return _DK_imp_will_soon_be_arming_trap(digger);
    dungeon = get_dungeon(traptng->owner);
    k = 0;
    i = dungeon->digger_list_start;
    while (i > 0)
    {
        thing = thing_get(i);
        if (thing_is_invalid(thing))
            break;
        cctrl = creature_control_get_from_thing(thing);
        i = cctrl->players_next_creature_idx;
        // Per-thing code
        if (cctrl->field_70 == traptng->index)
        {
            crstate = get_creature_state_besides_move(thing);
            if (crstate == CrSt_CreaturePicksUpTrapObject) {
                return true;
            }
            crstate = get_creature_state_besides_drag(thing);
            if (crstate == CrSt_CreatureArmsTrap) {
                return true;
            }
        }
        // Per-thing code ends
        k++;
        if (k > THINGS_COUNT)
        {
            ERRORLOG("Infinite loop detected when sweeping things list");
            break;
        }
    }
    return false;
}

struct Thing *check_for_empty_trap_for_imp_not_being_armed(struct Thing *digger, long trpmodel)
{
    struct Thing *thing;
    long i;
    unsigned long k;
    //return _DK_check_for_empty_trap_for_imp_not_being_armed(thing, a2);
    k = 0;
    i = game.thing_lists[TngList_Traps].index;
    while (i > 0)
    {
        thing = thing_get(i);
        if (thing_is_invalid(thing))
          break;
        i = thing->next_of_class;
        // Per-thing code
        if ( (thing->model == trpmodel) && (thing->byte_13 == 0) && (thing->owner == digger->owner) )
        {
            if ( !imp_will_soon_be_arming_trap(thing) )
            {
                return thing;
            }
        }
        // Per-thing code ends
        k++;
        if (k > THINGS_COUNT)
        {
          ERRORLOG("Infinite loop detected when sweeping things list");
          break;
        }
    }
    return INVALID_THING;
}

long check_out_unprettied_or_unconverted_area(struct Thing *thing)
{
    SYNCDBG(19,"Starting");
    return _DK_check_out_unprettied_or_unconverted_area(thing);
}

long check_out_unconverted_spiral(struct Thing *thing, long a2)
{
  return _DK_check_out_unconverted_spiral(thing, a2);
}

TbBool check_out_unprettied_spot(struct Thing *thing, long slb_x, long slb_y)
{
    long stl_x,stl_y;
    if ((slb_x >= 0) && (slb_x < map_tiles_x) && (slb_y >= 0) && (slb_y < map_tiles_y))
    {
      if (check_place_to_pretty_excluding(thing, slb_x, slb_y))
      {
          stl_x = 3*slb_x+1;
          stl_y = 3*slb_y+1;
          if (!imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
          {
              if (setup_person_move_to_position(thing, stl_x, stl_y, 0))
              {
                  thing->continue_state = CrSt_ImpArrivesAtImproveDungeon;
                  return true;
              }
          }
      }
    }
    return false;
}

long check_out_unprettied_spiral(struct Thing *thing, long nslabs)
{
    const struct Around *arnd;
    long slb_x,slb_y;
    long slabi,arndi;
    long i,imax,k;
    SYNCDBG(9,"Starting");
    //return _DK_check_out_unprettied_spiral(thing, nslabs);

    slb_x = map_to_slab[thing->mappos.x.stl.num];
    slb_y = map_to_slab[thing->mappos.y.stl.num];
    imax = 2;
    arndi = ACTION_RANDOM(4);
    for (slabi = 0; slabi < nslabs; slabi++)
    {
        {
          arnd = &small_around[arndi];
          {
              slb_x += arnd->delta_x;
              slb_y += arnd->delta_y;
              if (check_out_unprettied_spot(thing, slb_x, slb_y))
              {
                  return 1;
              }
          }
          arndi = (arndi + 1) & 3;
          i = 1;
        }
        for (k = 0; k < 4; k++)
        {
          arnd = &small_around[arndi];
          for (; i < imax; i++)
          {
              slb_x += arnd->delta_x;
              slb_y += arnd->delta_y;
              if (check_out_unprettied_spot(thing, slb_x, slb_y))
              {
                  return 1;
              }
          }
          arndi = (arndi + 1) & 3;
          i = 0;
        }
        imax += 2;
    }
    return 0;
}

long check_place_to_convert_excluding(struct Thing *thing, long a2, long a3)
{
  return _DK_check_place_to_convert_excluding(thing, a2, a3);
}

long check_place_to_pretty_excluding(struct Thing *thing, long a2, long a3)
{
    SYNCDBG(19,"Starting");
    return _DK_check_place_to_pretty_excluding(thing, a2, a3);
}

long check_out_unreinforced_place(struct Thing *thing)
{
  return _DK_check_out_unreinforced_place(thing);
}

long check_out_unreinforced_area(struct Thing *thing)
{
  return _DK_check_out_unreinforced_area(thing);
}

TbBool check_out_unconverted_place(struct Thing *thing)
{
    long stl_x,stl_y;
    long slb_x,slb_y;
    SYNCDBG(19,"Starting");
    slb_x = map_to_slab[thing->mappos.x.stl.num];
    slb_y = map_to_slab[thing->mappos.y.stl.num];
    stl_x = 3*slb_x + 1;
    stl_y = 3*slb_y + 1;
    if ( check_place_to_convert_excluding(thing, slb_x, slb_y)
      && !imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y) )
    {
        if (setup_person_move_to_position(thing, stl_x, stl_y, 0))
        {
            thing->continue_state = CrSt_ImpArrivesAtConvertDungeon;
            return true;
        }
    }
    if ( check_out_unconverted_spiral(thing, 1) )
    {
      return true;
    }
    return false;
}

long check_out_unprettied_place(struct Thing *thing)
{
  long stl_x,stl_y;
  long slb_x,slb_y;
  SYNCDBG(19,"Starting");
  slb_x = map_to_slab[thing->mappos.x.stl.num];
  slb_y = map_to_slab[thing->mappos.y.stl.num];
  stl_x = 3*slb_x + 1;
  stl_y = 3*slb_y + 1;
  if ( check_place_to_pretty_excluding(thing, slb_x, slb_y)
    && !imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y) )
  {
      if (setup_person_move_to_position(thing, stl_x, stl_y, 0))
      {
          thing->continue_state = CrSt_ImpArrivesAtImproveDungeon;
          return true;
      }
  }
  if ( check_out_unprettied_spiral(thing, 1) )
  {
    return true;
  }
  return false;
}

long check_out_undug_place(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    struct MapTask* mtask;
    SubtlCodedCoords task_pos;
    long task_idx;
    long stl_x,stl_y;
    long mv_x,mv_y;
    long i,n;
    SYNCDBG(19,"Starting");
    //return _DK_check_out_undug_place(thing);
    cctrl = creature_control_get_from_thing(thing);
    stl_x = stl_num_decode_x(cctrl->word_8F);
    stl_y = stl_num_decode_y(cctrl->word_8F);
    n = ACTION_RANDOM(4);
    for (i=0; i < 4; i++)
    {
        task_pos = get_subtile_number(3*(map_to_slab[stl_x]+small_around[n].delta_x) + 1,
                                      3*(map_to_slab[stl_y]+small_around[n].delta_y) + 1);
        task_idx = find_dig_from_task_list(thing->owner, task_pos);
        if (task_idx != -1)
        {
            mv_x = 0; mv_y = 0;
            if (check_place_to_dig_and_get_position(thing, task_pos, &mv_x, &mv_y)
                && setup_person_move_to_position(thing, mv_x, mv_y, 0))
            {
                cctrl->word_91 = task_idx;
                cctrl->word_8F = task_pos;
                mtask = get_task_list_entry(thing->owner, cctrl->word_91);
                if (mtask->field_0 == 2)
                {
                  thing->continue_state = CrSt_ImpArrivesAtDigOrMine2;
                } else
                {
                  thing->continue_state = CrSt_ImpArrivesAtDigOrMine1;
                }
                return 1;
            }
        }
        n = (n + 1) % 4;
    }
    return 0;
}

long check_out_undug_area(struct Thing *thing)
{
    SYNCDBG(19,"Starting");
    return _DK_check_out_undug_area(thing);
}

long add_undug_to_imp_stack(struct Dungeon *dungeon, long num)
{
    struct MapTask* mtask;
    long stl_x, stl_y;
    long i,nused;
    SYNCDBG(18,"Starting");
    nused = 0;
    i = -1;
    while ((num > 0) && (dungeon->digger_stack_length < IMP_TASK_MAX_COUNT))
    {
        i = find_next_dig_in_dungeon_task_list(dungeon, i);
        if (i < 0)
            break;
        mtask = get_dungeon_task_list_entry(dungeon, i);
        stl_x = stl_num_decode_x(mtask->field_1);
        stl_y = stl_num_decode_y(mtask->field_1);
        if ( subtile_revealed(stl_x, stl_y, dungeon->owner) )
        {
          if ( block_has_diggable_side(dungeon->owner, map_to_slab[stl_x], map_to_slab[stl_y]) )
          {
            add_to_imp_stack_using_pos(mtask->field_1, DigTsk_DigOrMine, dungeon);
            num--;
            nused++;
          }
        }
    }
    return nused;
}

void add_pretty_and_convert_to_imp_stack(struct Dungeon *dungeon)
{
  SYNCDBG(18,"Starting");
//TODO: rework! (causes hang if near egde of the map)
  _DK_add_pretty_and_convert_to_imp_stack(dungeon); return;
}

long add_unclaimed_gold_to_imp_stack(struct Dungeon *dungeon)
{
  return _DK_add_unclaimed_gold_to_imp_stack(dungeon);
}

void setup_imp_stack(struct Dungeon *dungeon)
{
  long i;
  for (i = 0; i < dungeon->digger_stack_length; i++)
  {
    dungeon->imp_stack[i].task_id = DigTsk_None;
  }
  dungeon->digger_stack_update_turn = game.play_gameturn;
  dungeon->digger_stack_length = 0;
  r_stackpos = 0;
}

long add_unclaimed_unconscious_bodies_to_imp_stack(struct Dungeon *dungeon, long a2)
{
  return _DK_add_unclaimed_unconscious_bodies_to_imp_stack(dungeon, a2);
}

TbBool add_unclaimed_dead_bodies_to_imp_stack(struct Dungeon *dungeon, long max_tasks)
{
    struct Thing *thing;
    struct Room *room;
    SubtlCodedCoords stl_num;
    int remain_num;
    unsigned long k;
    int i;
    //return _DK_add_unclaimed_dead_bodies_to_imp_stack(dungeon, max_tasks);
    if (dungeon->room_kind[RoK_GRAVEYARD] <= 0) {
        SYNCDBG(8,"Dungeon %d has no graveyard",(int)dungeon->owner);
        return 1;
    }
    room = find_room_with_spare_capacity(dungeon->owner, RoK_GRAVEYARD, 1);
    k = 0;
    i = game.thing_lists[TCls_DeadCreature].index;
    remain_num = max_tasks;
    while (i != 0)
    {
        thing = thing_get(i);
        if (thing_is_invalid(thing))
        {
            ERRORLOG("Jump to invalid thing detected");
            break;
        }
        i = thing->next_of_class;
        if ( (dungeon->digger_stack_length >= IMP_TASK_MAX_COUNT) || (remain_num <= 0) ) {
            break;
        }
        if ( ((thing->field_1 & 0x01) == 0) && (thing->active_state == 2) && (thing->byte_14 == 0) && corpse_is_rottable(thing) )
        {
            if (room_is_invalid(room))
            {
                SYNCDBG(8,"Dungeon %d has no free graveyard space",(int)dungeon->owner);
                if (is_my_player_number(dungeon->owner)) {
                    output_message(SMsg_GraveyardTooSmall, 1000, true);
                }
                return 0;
            }
            if ( subtile_revealed(thing->mappos.x.stl.num,thing->mappos.y.stl.num,dungeon->owner) )
            {
                stl_num = get_subtile_number(thing->mappos.x.stl.num,thing->mappos.y.stl.num);
                add_to_imp_stack_using_pos(stl_num, DigTsk_PickUpCorpse, dungeon);
                remain_num--;
            }
        }
        k++;
        if (k > THINGS_COUNT)
        {
            ERRORLOG("Infinite loop detected when sweeping things list");
            break;
        }
    }
    SYNCDBG(8,"Done, added %d tasks",(int)(max_tasks-remain_num));
    return 1;
}

long add_unclaimed_spells_to_imp_stack(struct Dungeon *dungeon, long a2)
{
  return _DK_add_unclaimed_spells_to_imp_stack(dungeon, a2);
}

long add_object_for_trap_to_imp_stack(struct Dungeon *dungeon, struct Thing *thing)
{
  return _DK_add_object_for_trap_to_imp_stack(dungeon, thing);
}

TbBool add_empty_traps_to_imp_stack(struct Dungeon *dungeon, long num)
{
  struct Thing *thing;
  unsigned long k;
  int i;
  SYNCDBG(18,"Starting");
  k = 0;
  i = game.thing_lists[7].index;
  while (i != 0)
  {
    thing = thing_get(i);
    if (thing_is_invalid(thing))
    {
      ERRORLOG("Jump to invalid thing detected");
      break;
    }
    i = thing->next_of_class;
    // Thing list loop body
    if ((num <= 0) || (dungeon->digger_stack_length >= IMP_TASK_MAX_COUNT))
      break;
    if ((!thing->byte_13) && (thing->owner == dungeon->owner))
    {
      if ( add_object_for_trap_to_imp_stack(dungeon, thing) )
        num--;
    }
    // Thing list loop body ends
    k++;
    if (k > THINGS_COUNT)
    {
      ERRORLOG("Infinite loop detected when sweeping things list");
      break;
    }
  }
  SYNCDBG(19,"Finished");
  return true;
}

TbBool add_unclaimed_traps_to_imp_stack(struct Dungeon *dungeon)
{
  struct SlabMap* slb;
  struct Room* room;
  unsigned long stl_num;
  struct Thing* thing;
  unsigned long k;
  int i;
  SYNCDBG(18,"Starting");
  // Checking if the workshop exists
  room = find_room_with_spare_room_item_capacity(dungeon->owner, RoK_WORKSHOP);
  if ( (dungeon->room_kind[RoK_WORKSHOP] <= 0) || room_is_invalid(room) )
    return false;
  k = 0;
  i = game.thing_lists[2].index;
  while (i != 0)
  {
    thing = thing_get(i);
    if (thing_is_invalid(thing))
    {
      ERRORLOG("Jump to invalid thing detected");
      break;
    }
    i = thing->next_of_class;
    // Thing list loop body
    if (dungeon->digger_stack_length >= IMP_TASK_MAX_COUNT)
      break;
    if ( thing_is_door_or_trap_box(thing) )
    {
      if ((thing->field_1 & 0x01) == 0)
      {
        if ((thing->owner == dungeon->owner) || (thing->owner == game.neutral_player_num))
        {
          slb = get_slabmap_for_subtile(thing->mappos.x.stl.num, thing->mappos.y.stl.num);
          if (slabmap_owner(slb) == dungeon->owner)
          {
            room = get_room_thing_is_on(thing);
            if (room_is_invalid(room) || (room->kind != RoK_WORKSHOP))
            {
              stl_num = get_subtile_number(thing->mappos.x.stl.num,thing->mappos.y.stl.num);
              add_to_imp_stack_using_pos(stl_num, DigTsk_PicksUpTrapForWorkshop, dungeon);
            }
          }
        }
      }
    }
    // Thing list loop body ends
    k++;
    if (k > THINGS_COUNT)
    {
      ERRORLOG("Infinite loop detected when sweeping things list");
      break;
    }
  }
  SYNCDBG(19,"Finished");
  return true;
}

void add_reinforce_to_imp_stack(struct Dungeon *dungeon)
{
    struct DiggerStack *rfstack;
    long i;
    for (i=0; i < r_stackpos; i++)
    {
        if (dungeon->digger_stack_length >= IMP_TASK_MAX_COUNT)
          break;
        rfstack = &reinforce_stack[i];
        add_to_imp_stack_using_pos(rfstack->field_0, rfstack->task_id, dungeon);
    }
}

long check_out_uncrowded_reinforce_position(struct Thing *thing, unsigned short a2, long *a3, long *a4)
{
    return _DK_check_out_uncrowded_reinforce_position(thing, a2, a3, a4);
}

long check_place_to_dig_and_get_position(struct Thing *thing, unsigned long stl_num, long *retstl_x, long *retstl_y)
{
    struct SlabMap *place_slb;
    struct Coord3d pos;
    long place_x,place_y;
    long distance_x,distance_y;
    long base_x,base_y;
    long stl_x,stl_y;
    long i,k,n,nstart;
    SYNCDBG(18,"Starting");
    //return _DK_check_place_to_dig_and_get_position(thing, stl_num, retstl_x, retstl_y);
    place_x = stl_num_decode_x(stl_num);
    place_y = stl_num_decode_y(stl_num);
    if (!block_has_diggable_side(thing->owner, map_to_slab[place_x], map_to_slab[place_y]))
        return 0;
    distance_x = place_x - thing->mappos.x.stl.num;
    distance_y = place_y - thing->mappos.y.stl.num;
    if (abs(distance_y) >= abs(distance_x))
    {
      if (distance_y > 0)
          nstart = 0;
      else
          nstart = 2;
    } else
    {
      if (distance_x > 0)
          nstart = 3;
      else
          nstart = 1;
    }
    place_slb = get_slabmap_for_subtile(place_x,place_y);
    n = nstart;

    for (i = 0; i < SMALL_AROUND_SLAB_LENGTH; i++)
    {
      base_x = place_x + 2 * (long)small_around[n].delta_x;
      base_y = place_y + 2 * (long)small_around[n].delta_y;
      if (valid_dig_position(thing->owner, base_x, base_y))
      {
          for (k = 0; k < sizeof(dig_pos)/sizeof(dig_pos[0]); k++)
          {
              if ( k )
              {
                nstart = ((n + dig_pos[k]) & 3);
                stl_x = base_x + small_around[nstart].delta_x;
                stl_y = base_y + small_around[nstart].delta_y;
              } else
              {
                stl_x = base_x;
                stl_y = base_y;
              }
              if (valid_dig_position(thing->owner, stl_x, stl_y))
              {
                    if ((place_slb->kind != SlbT_GEMS) || !gold_pile_with_maximum_at_xy(stl_x, stl_y))
                      if (!imp_already_digging_at_excluding(thing, stl_x, stl_y))
                        if (!imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
                        {
                          set_coords_to_subtile_center(&pos, stl_x, stl_y, 0);
                          pos.z.val = get_thing_height_at(thing, &pos);
                          if (creature_can_navigate_to_with_storage(thing, &pos, 0))
                          {
                              *retstl_x = stl_x;
                              *retstl_y = stl_y;
                              return 1;
                          }
                        }
              }
          }
      }
      n = (n+1) % 4;
    }
    return 0;
}

struct Thing *check_place_to_pickup_dead_body(struct Thing *thing, long stl_x, long stl_y)
{
    return _DK_check_place_to_pickup_dead_body(thing, stl_x, stl_y);
}

struct Thing *check_place_to_pickup_gold(struct Thing *thing, long stl_x, long stl_y)
{
    return _DK_check_place_to_pickup_gold(thing, stl_x, stl_y);
}

struct Thing *check_place_to_pickup_spell(struct Thing *thing, long a2, long a3)
{
    return _DK_check_place_to_pickup_spell(thing, a2, a3);
}

struct Thing *check_place_to_pickup_unconscious_body(struct Thing *thing, long a2, long a3)
{
    return _DK_check_place_to_pickup_unconscious_body(thing, a2, a3);
}

long check_place_to_reinforce(struct Thing *thing, long a2, long a3)
{
    return _DK_check_place_to_reinforce(thing, a2, a3);
}

struct Thing *check_place_to_pickup_crate(struct Thing *thing, long stl_x, long stl_y)
{
    return _DK_check_place_to_pickup_crate(thing, stl_x, stl_y);
}

/**
 * Checks if given digger has money that should be placed in treasure room.
 * If he does, he is ordered to return them into nearest treasure room
 * which has the proper capacity. If there's no proper treasure room,
 * a proper speech message is created.
 * @param thing The digger creature.
 * @return Gives 1 if the digger was ordered to go into treasure room, 0 otherwise.
 */
long check_out_imp_has_money_for_treasure_room(struct Thing *thing)
{
    struct Dungeon *dungeon;
    struct Room *room;
    SYNCDBG(18,"Starting");
    //If the imp doesn't have any money - then just return
    if (thing->creature.gold_carried <= 0)
      return 0;
    // Find a treasure room to drop the money
    room = find_nearest_room_for_thing_with_spare_capacity(thing, thing->owner, RoK_TREASURE, 0, 1);
    if (room != NULL)
    {
        if (setup_head_for_empty_treasure_space(thing, room))
        {
          thing->continue_state = CrSt_ImpDropsGold;
          return 1;
        }
        return 0;
    }
    dungeon = get_dungeon(thing->owner);
    // Check why the treasure room search failed. Maybe we don't have treasure room?
    if (dungeon->room_kind[RoK_TREASURE] == 0)
    {
        if (is_my_player_number(thing->owner))
            output_message(SMsg_RoomTreasrNeeded, 1000, true);
        event_create_event_or_update_nearby_existing_event(0, 0, 20, thing->owner, 0);
        return 0;
    }
    // If we have it, is it unreachable, or just too small?
    if (find_room_with_spare_capacity(thing->owner, RoK_TREASURE, 1) != NULL)
    {
        if (is_my_player_number(thing->owner))
            output_message(SMsg_NoRouteToTreasury, 1000, true);
        return 0;
    }
    if (is_my_player_number(thing->owner))
        output_message(SMsg_TreasuryTooSmall, 1000, true);
    event_create_event_or_update_nearby_existing_event(0, 0, 11, thing->owner, 0);
    return 0;
}

long check_out_available_imp_tasks(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    SYNCDBG(9,"Starting");
    cctrl = creature_control_get_from_thing(thing);
    imp_stack_update(thing);
    if ( check_out_imp_stack(thing) )
        return 1;
    if (game.play_gameturn-cctrl->field_2C7 > 128)
    {
        check_out_imp_has_money_for_treasure_room(thing);
        cctrl->field_2C7 = game.play_gameturn;
        return 1;
    }
    return 0;
}

long check_out_imp_tokes(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    long i;
    SYNCDBG(19,"Starting");
    cctrl = creature_control_get_from_thing(thing);
    i = ACTION_RANDOM(64);
    // small chance of changing state
    if (i != 0)
      return 0;
    internal_set_thing_state(thing, CrSt_ImpToking);
    thing->continue_state = CrSt_ImpDoingNothing;
    cctrl->field_282 = 200;
    return 1;
}

long check_out_imp_last_did(struct Thing *thing)
{
  struct CreatureControl *cctrl;
  struct Dungeon *dungeon;
  struct Room *room;
  //return _DK_check_out_imp_last_did(thing);
  cctrl = creature_control_get_from_thing(thing);
  SYNCDBG(19,"Starting for %s index %d, last did %d",thing_model_name(thing),(int)thing->index,(int)cctrl->digger.last_did_job);
  switch (cctrl->digger.last_did_job)
  {
  case 0:
      return false;
  case 1:
      if ( check_out_undug_place(thing) || check_out_undug_area(thing) )
      {
        cctrl->digger.last_did_job = 1;
        return true;
      }
      if ( check_out_unconverted_place(thing) || check_out_unprettied_place(thing) )
      {
        cctrl->digger.last_did_job = 2;
        return true;
      }
      imp_stack_update(thing);
      if ( check_out_unprettied_or_unconverted_area(thing) )
      {
        cctrl->digger.last_did_job = 2;
        SYNCDBG(19,"Done on unprettied or unconverted area");
        return true;
      }
      break;
  case 2:
      if ( check_out_unconverted_place(thing) || check_out_unprettied_place(thing) )
      {
        cctrl->digger.last_did_job = 2;
        SYNCDBG(19,"Done on unprettied or unconverted place");
        return true;
      }
      imp_stack_update(thing);
      if ( check_out_unprettied_or_unconverted_area(thing) )
      {
        cctrl->digger.last_did_job = 2;
        return true;
      }
      if ( check_out_undug_area(thing) )
      {
        cctrl->digger.last_did_job = 1;
        return true;
      }
      break;
  case 3:
      dungeon = get_dungeon(thing->owner);
      imp_stack_update(thing);
      if ((dungeon->digger_stack_update_turn != cctrl->digger.stack_update_turn) && (dungeon->digger_stack_length != 3))
        break;
      if ( check_out_unreinforced_place(thing) )
      {
        cctrl->digger.last_did_job = 3;
        return true;
      }
      if ( check_out_unreinforced_area(thing) )
      {
        cctrl->digger.last_did_job = 3;
        return true;
      }
      break;
  case 4:
      if ( !creature_can_be_trained(thing) || !player_can_afford_to_train_creature(thing) )
        break;
      room = find_nearest_room_for_thing_with_spare_capacity(thing, thing->owner, 6, 0, 1);
      if (!room_is_invalid(room))
      {
        if ( setup_random_head_for_room(thing, room, 0) )
        {
          thing->continue_state = CrSt_AtTrainingRoom;
          cctrl->field_80 = room->index;
          return true;
        }
      }
      if (is_my_player_number(thing->owner))
      {
        if ( !find_room_with_spare_capacity(thing->owner, 6, 1) )
          output_message(SMsg_TrainingTooSmall, 0, true);
      }
      break;
  case 9:
      if ( check_out_unreinforced_place(thing) )
      {
        cctrl->digger.last_did_job = 9;
        return true;
      }
      if ( check_out_unreinforced_area(thing) )
      {
        cctrl->digger.last_did_job = 9;
        return true;
      }
      break;
  default:
      break;
  }
  cctrl->digger.last_did_job = 0;
  return false;
}

long imp_stack_update(struct Thing *thing)
{
  struct Dungeon *dungeon;
  SYNCDBG(18,"Starting");
  //return _DK_imp_stack_update(thing);
  dungeon = get_dungeon(thing->owner);
  if ((game.play_gameturn - dungeon->digger_stack_update_turn) < 128)
    return 0;
  SYNCDBG(8,"Updating");
  setup_imp_stack(dungeon);
  add_unclaimed_unconscious_bodies_to_imp_stack(dungeon, IMP_TASK_MAX_COUNT/4 - 1);
  add_unclaimed_dead_bodies_to_imp_stack(dungeon, IMP_TASK_MAX_COUNT/4 - 1);
  add_unclaimed_spells_to_imp_stack(dungeon, IMP_TASK_MAX_COUNT/12);
  add_empty_traps_to_imp_stack(dungeon, IMP_TASK_MAX_COUNT/6);
  add_undug_to_imp_stack(dungeon, IMP_TASK_MAX_COUNT*5/8);
  add_pretty_and_convert_to_imp_stack(dungeon);
  add_unclaimed_gold_to_imp_stack(dungeon);
  add_unclaimed_traps_to_imp_stack(dungeon);
  add_reinforce_to_imp_stack(dungeon);
  return 1;
}

long check_out_imp_stack(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    struct Dungeon *dungeon;
    struct CreatureStats *crstat;
    struct Thing *sectng;
    struct Thing *trdtng;
    struct DiggerStack *istack;
    struct MapTask *task;
    long stl_x,stl_y;
    long i;
    SYNCDBG(18,"Starting");
    //return _DK_check_out_imp_stack(thing);
    cctrl = creature_control_get_from_thing(thing);
    dungeon = get_dungeon(thing->owner);
    if (cctrl->digger.stack_update_turn != dungeon->digger_stack_update_turn)
    {
      cctrl->digger.stack_update_turn = dungeon->digger_stack_update_turn;
      cctrl->field_95 = 0;
    }
    if (dungeon->digger_stack_length > IMP_TASK_MAX_COUNT)
    {
        ERRORLOG("Imp tasks length %d out of range",(int)dungeon->digger_stack_length);
        dungeon->digger_stack_length = IMP_TASK_MAX_COUNT;
    }
    while (cctrl->field_95 < dungeon->digger_stack_length)
    {
        istack = &dungeon->imp_stack[cctrl->field_95];
        cctrl->field_95++;
        SYNCDBG(18,"Checking task %d",(int)istack->task_id);
        switch (istack->task_id)
        {
        case DigTsk_ImproveDungeon:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if ( !check_place_to_pretty_excluding(thing, map_to_slab[stl_x], map_to_slab[stl_y]) )
            {
                istack->task_id = DigTsk_None;
                break;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
              break;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_ImpArrivesAtImproveDungeon;
            cctrl->digger.last_did_job = 2;
            return 1;

        case DigTsk_ConvertDungeon:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (!check_place_to_convert_excluding(thing, map_to_slab[stl_x], map_to_slab[stl_y]))
            {
                istack->task_id = DigTsk_None;
                break;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
              break;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_ImpArrivesAtConvertDungeon;
            cctrl->digger.last_did_job = 2;
            return 1;

        case DigTsk_ReinforceWall:
            if (game.play_gameturn - cctrl->field_2C7 > 128)
            {
              cctrl->field_95--;
              check_out_imp_has_money_for_treasure_room(thing);
              cctrl->field_2C7 = game.play_gameturn;
              return 1;
            }
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (check_place_to_reinforce(thing, map_to_slab[stl_x], map_to_slab[stl_y]) <= 0)
            {
                istack->task_id = DigTsk_None;
                break;
            }
            if (!check_out_uncrowded_reinforce_position(thing, istack->field_0, &stl_x, &stl_y))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0) )
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_ImpArrivesAtReinforce;
            cctrl->digger.byte_93 = 0;
            cctrl->word_8D = istack->field_0;
            cctrl->digger.last_did_job = 3;
            return 1;

        case DigTsk_PickUpUnconscious:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (!player_has_room(dungeon->owner,RoK_PRISON) ||
                !player_creature_tends_to(dungeon->owner,CrTend_Imprison))
              break;
            if (!find_nearest_room_for_thing_with_spare_capacity(thing, thing->owner, 4, 0, 1))
            {
              if (is_my_player_number(thing->owner))
              {
                if (!find_room_with_spare_capacity(thing->owner, 4, 1))
                  output_message(SMsg_PrisonTooSmall, 1000, true);
              }
              istack->task_id = DigTsk_None;
              return -1;
            }
            sectng = check_place_to_pickup_unconscious_body(thing, stl_x, stl_y);
            if (thing_is_invalid(sectng))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0) )
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_CreaturePickUpUnconsciousBody;
            cctrl->field_74 = sectng->index;
            return 1;

        case DigTsk_PickUpCorpse:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (dungeon->room_kind[11] == 0)
              break;
            if ( find_nearest_room_for_thing_with_spare_capacity(thing, thing->owner, 11, 0, 1) )
            {
              if (is_my_player_number(thing->owner))
              {
                if (!find_room_with_spare_capacity(thing->owner, 11, 1))
                  output_message(SMsg_GraveyardTooSmall, 1000, true);
              }
              istack->task_id = DigTsk_None;
              return -1;
            }
            sectng = check_place_to_pickup_dead_body(thing, stl_x, stl_y);
            if (thing_is_invalid(sectng))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_CreaturePicksUpCorpse;
            cctrl->field_72 = sectng->index;
            return 1;

        case DigTsk_PicksUpSpellBook:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (dungeon->room_kind[3] == 0)
                break;
            if (!find_nearest_room_for_thing_with_spare_item_capacity(thing, thing->owner, 3, 0))
            {
                if (is_my_player_number(thing->owner))
                {
                  if (!find_room_with_spare_room_item_capacity(thing->owner, 3))
                    output_message(SMsg_LibraryTooSmall, 1000, true);
                }
                istack->task_id = DigTsk_None;
                return -1;
            }
            sectng = check_place_to_pickup_spell(thing, stl_x, stl_y);
            if (thing_is_invalid(sectng))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0) )
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (thing_is_spellbook(sectng))
            {
                event_create_event_or_update_nearby_existing_event(
                    get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                    14, thing->owner, sectng->index);
            } else
            if (thing_is_special_box(sectng))
            {
                event_create_event_or_update_nearby_existing_event(
                    get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                    26, thing->owner, sectng->index);
            } else
            {
                WARNLOG("Strange pickup (model %d) - no event",(int)sectng->model);
            }
            thing->continue_state = CrSt_CreaturePicksUpSpellObject;
            cctrl->field_72 = sectng->index;
            return 1;

        case DigTsk_PicksUpTrapBox:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            sectng = check_place_to_pickup_crate(thing, stl_x, stl_y);
            if (thing_is_invalid(sectng))
            {
                istack->task_id = DigTsk_None;
                break;
            }
            if (!thing_is_trap_box(sectng))
            {
                break;
            }
            trdtng = check_for_empty_trap_for_imp_not_being_armed(thing, box_thing_to_door_or_trap(sectng));
            if (thing_is_invalid(trdtng))
            {
                break;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
              break;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
              istack->task_id = DigTsk_None;
              return -1;
            }
            thing->continue_state = CrSt_CreaturePicksUpTrapObject;
            cctrl->field_72 = sectng->index;
            cctrl->field_70 = trdtng->index;
            return 1;

        case DigTsk_PicksUpTrapForWorkshop:
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (dungeon->room_kind[8] == 0)
            {
              break;
            }
            if (!find_nearest_room_for_thing_with_spare_item_capacity(thing, thing->owner, 8, 0))
            {
              if (is_my_player_number(thing->owner))
              {
                if (!find_room_with_spare_room_item_capacity(thing->owner, 8))
                  output_message(SMsg_WorkshopTooSmall, 1000, true);
              }
              istack->task_id = DigTsk_None;
              return -1;
            }
            sectng = check_place_to_pickup_crate(thing, stl_x, stl_y);
            if (thing_is_invalid(sectng))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            i = workshop_object_class[sectng->model];
            if ( i == 8 )
            {
              event_create_event_or_update_nearby_existing_event(
                  get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                  24, thing->owner, sectng->index);
            } else
            if (i == 9)
            {
              event_create_event_or_update_nearby_existing_event(
                  get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                  25, thing->owner, sectng->index);
            } else
            {
                WARNLOG("Strange pickup (class %d) - no event",(int)i);
            }
            thing->continue_state = CrSt_CreaturePicksUpTrapForWorkshop;
            cctrl->field_72 = sectng->index;
            return 1;

        case DigTsk_DigOrMine:
            i = find_dig_from_task_list(thing->owner, istack->field_0);
            if (i == -1)
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            stl_x = 0; stl_y = 0;
            if (!check_place_to_dig_and_get_position(thing, istack->field_0, &stl_x, &stl_y)
              || !setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
              istack->task_id = DigTsk_None;
              return -1;
            }
            cctrl->word_91 = i;
            cctrl->word_8F = istack->field_0;
            cctrl->digger.last_did_job = 1;
            task = get_dungeon_task_list_entry(dungeon, i);
            if (task->field_0 == 2)
            {
              thing->continue_state = CrSt_ImpArrivesAtDigOrMine2;
            } else
            {
              thing->continue_state = CrSt_ImpArrivesAtDigOrMine1;
            }
            return 1;

        case DigTsk_PicksUpGoldPile:
            crstat = creature_stats_get_from_thing(thing);
            if (crstat->gold_hold <= thing->creature.gold_carried)
            {
                if (game.play_gameturn - cctrl->field_2C7 > 128)
                {
                  check_out_imp_has_money_for_treasure_room(thing);
                  cctrl->field_2C7 = game.play_gameturn;
                }
                return 1;
            }
            stl_x = stl_num_decode_x(istack->field_0);
            stl_y = stl_num_decode_y(istack->field_0);
            if (!check_place_to_pickup_gold(thing, stl_x, stl_y))
            {
                istack->task_id = DigTsk_None;
                break;
            }
            if (imp_will_soon_be_working_at_excluding(thing, stl_x, stl_y))
            {
                break;
            }
            if (!setup_person_move_to_position(thing, stl_x, stl_y, 0))
            {
                istack->task_id = DigTsk_None;
                return -1;
            }
            thing->continue_state = CrSt_ImpPicksUpGoldPile;
            return 1;

        case DigTsk_None:
            break;
        default:
            ERRORLOG("Invalid stack type, %d",(int)istack->task_id);
            istack->task_id = DigTsk_None;
            break;
        }
    }
    return 0;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
