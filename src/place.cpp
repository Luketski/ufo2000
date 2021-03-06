/*
This file is part of UFO2000 (http://ufo2000.sourceforge.net)

Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002       ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"

#include "global.h"
#include "video.h"
#include "cell.h"
#include "place.h"
#include "map.h"
#include "colors.h"
#include "text.h"
#include "explo.h"
#include "script_api.h"

IMPLEMENT_PERSISTENCE(Place, "Place");

Place::Place(int x, int y, int w, int h, Cell* cell)
{
    viscol = 0;
    m_item = NULL;
    set(x, y, w, h);
    m_cell = cell;
}

Place::~Place()
{
    Item *t1 = m_item, *t2 = NULL;
    while (t1 != NULL) {
        t2 = t1->m_next;	// remember pointer as unlink() deletes it
        t1->unlink();
        delete t1;
        t1 = t2;
    }
}

/**
 * Set size and position of place.
 * @param x position on screen, x
 * @param y position on screen, y
 */
void Place::set(int x, int y, int w, int h)
{
    gx = x; gy = y;
    width = w; height = h;
}

/**
 * Scroll items on the ground-grid left
 */
// Todo: repack items
// ?? maybe scroll several columns at once
void Place::scroll_left()
{
    if (viscol > 0) viscol--;
}

/**
 * Scroll items on the ground-grid right
 */
void Place::scroll_right()
{
    if (viscol < width - 1) viscol++;
}


Item *Place::item_under_mouse(int scr_x, int scr_y)
{
    int xx = (mouse_x - (scr_x + gx)) / 16;
    int yy = (mouse_y - (scr_y + gy)) / 15;

    if ((xx >= 0) && (xx < width) && (yy >= 0) && (yy < height)) {
        Item *t = m_item;
        while (t != NULL) {
            if (t->inside(xx, yy))
                return t;
            t = t->m_next;
        }
    }
    return NULL;
}

Item *Place::item(int ix, int iy)
{
    Item *t = m_item;
    while (t != NULL) {
        if ((ix == t->m_x) && (iy == t->m_y)) {
            return t;
        }
        t = t->m_next;
    }
    return NULL;
}

/**
 * Get most important item in place.
 * Used for drawing the playing area.
 */
Item *Place::top_item()
{
    if (m_item == NULL)
        return NULL;

    Item *t = m_item, *gt = m_item;

    while (t != NULL) {
        if (t->obdata_importance() > gt->obdata_importance()) {
            gt = t;
        }
        t = t->m_next;
    }

    return gt;
}

int Place::isfree(int xx, int yy)
{
    if (!outside_belt(xx, yy))
        if ((xx >= 0) && (xx < width) && (yy >= 0) && (yy < height)) {
            Item * t = m_item;
            while (t != NULL) {
                if (t->inside(xx, yy))
                    return 0;
                t = t->m_next;
                //if (t==m_item) return 0;
            }
            return 1;
        }
    return 0;
}

/**
 * Return true if this place is a soldier's hand.
 */
int Place::ishand()
{
    if ((width == 2) && (height == 3))
        return 1;
    return 0;
}

/**
 * Test if item fits into a place like hand, belt, backpack etc.
 * Return 1 if it fits, 0 otherwise
 */
int Place::isfit(Item * it, int xx, int yy)
{
    if (ishand() && (m_item != NULL))
        return 0;

    for (int i = xx; i < xx + it->obdata_width(); i++)
        for (int j = yy; j < yy + it->obdata_height(); j++)
            if (!isfree(i, j)) return 0;
    return 1;
}

/**
 * Put item in place like hand, belt, backpack etc.
 * @return 1 if successful
 */
int Place::put(Item *it, int xx, int yy)
{
    ASSERT(it->m_place == NULL);

    //If an item is dropped, make it fall down
    if (m_cell != NULL) {
        Position p = m_cell->get_position();
        if ((p.level() > 0) &&
            (map->mcd(p.level(), p.column(), p.row(), 0)->No_Floor) &&
            !(map->isStairs(p.level()-1, p.column(), p.row())))
                return map->place(p.level()-1, p.column(), p.row())->put(it, xx, yy);
    }


    if (isfree(xx, yy) && isfit(it, xx, yy)) {
        if (m_item != NULL)
            m_item->m_prev = it;
        it->m_next = m_item; it->m_prev = NULL; it->m_place = this;
        it->setpos(xx, yy);
        m_item = it;
       	/* if this place belongs to a map cell */
        if (m_cell != NULL) {
           	Position p = m_cell->get_position();
            map->update_seen_item(p);
            if (it->obdata_ownLight())
            	/* if item has own light - an electro flare */
				map->add_light_source(p.level(), p.column(), p.row(), it->obdata_ownLight());
        }

        return 1;
    }
    return 0;
}

/**
 * Put item in place like hand, belt, backpack etc.
 * This function also finds a suitable position for the item.
 * @return 1 if successful, 0 otherwise (no room for item).
 */
int Place::put(Item * it)
{
    it->unlink();	// delete item from its original position

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (put(it, x, y))
                return 1;
        }
    }
    return 0;
}

/**
 * Place stores item.
 * Used only to empty the place (set_item(NULL)); maybe FIXME remove func
 * or rename to Place::clear?
 */
void Place::set_item(Item *it)
{
    m_item = it;
    if (m_cell != NULL) {
       	/* if item has own light - an electro flare */
        if (int ownlight = it->obdata_ownLight()) {
        	Position p = m_cell->get_position();
			map->remove_light_source(p.level(), p.column(), p.row(), ownlight);
		}
        map->update_seen_item(m_cell->get_position());
    }

}

/**
 * Get item from place (delete from place's list)
 * @return The item at the position, or NULL if no item there.
 */
Item * Place::get(int xx, int yy)
{
    Item * t;
    t = m_item;
    while (t != NULL) {
        if (ishand() || t->inside(xx, yy)) {
            t->unlink();	/* remove from list */
            if (m_cell != NULL) {
            	Position p = m_cell->get_position();
                map->update_seen_item(p);
	            if (int ownlight = t->obdata_ownLight())
	            	/* if item has own light - an electro flare */
					map->remove_light_source(p.level(), p.column(), p.row(), ownlight);
			}
            return t;
        }
        t = t->m_next;
    }
    return NULL;
}

/**
 * Destroy an item.
 * @param it The item to destroy.
 * @return 1 if destroyed, 0 if not in list.
 */
int Place::destroy(Item *it)
{
    Item * t;
    t = m_item;
    while (t != NULL) {
        if (t == it) {
            t->unlink();	// remove from list
            if (m_cell != NULL) {
            	Position p = m_cell->get_position();
                map->update_seen_item(p);
	            if (int ownlight = t->obdata_ownLight())	/* if item has own light - an electro flare */
					map->remove_light_source(p.level(), p.column(), p.row(), ownlight);
			}
            delete it;
            return 1;
        }
        t = t->m_next;
    }
    return 0;
}

/**
 * Pick item from place with mouse.
 * @return The item, if found, NULL otherwise.
 */
Item *Place::mselect(int scr_x, int scr_y)
{
    if ((mouse_x > scr_x + gx) && (mouse_x < scr_x + gx + width * 16))
        if ((mouse_y > scr_y + gy) && (mouse_y < scr_y + gy + height * 15))
            return get((mouse_x - (scr_x + gx)) / 16 + viscol, (mouse_y - (scr_y + gy)) / 15);
    return NULL;
}

/**
 * Put item to place with mouse.
 * @return 1 if successful, 0 otherwise.
 */
int Place::mdeselect(Item *it, int scr_x, int scr_y)
{
    if ((mouse_x > scr_x + gx) && (mouse_x < scr_x + gx + width * 16))
        if ((mouse_y > scr_y + gy) && (mouse_y < scr_y + gy + height * 15)) {
            int x2 = (mouse_x - (scr_x + gx) - (it->obdata_width() - 1) * 8) / 16 + viscol;
            int y2 = (mouse_y - (scr_y + gy) - (it->obdata_height() - 1) * 8) / 15;
            //text_mode(0);
            //textprintf(screen, font, 1, 150, 1, "x=%d y=%d", x2, y2);

            if (isfree(x2, y2) && isfit(it, x2, y2)) {
                put(it, x2, y2);
                return 1;
            }
        }
    return 0;
}

/**
 * Check if the given position does not fit in a soldier's belt, if this place is a belt anyway.
 * @return true if would not fit
 */
int Place::outside_belt(int x, int y)
{
    if ((width == 4) && (height == 2))
        if ((y == 1) && ((x == 1) || (x == 2)))
            return 1;
    return 0;
}

/**
 * Draw item at a place on the battlemap.
 * If there are several items, draw the item with the highest importance.
 */
void Place::draw(int gx, int gy)
{
    if (m_item == NULL)
        return ;

    Item *t = m_item, *gt = m_item;

    while (t != NULL) {
        if (t->obdata_importance() > gt->obdata_importance()) {
            gt = t;
        }
        t = t->m_next;
    }

    map->drawitem(gt->obdata_pMap(), gx, gy);
}

/**
 * Draw inventory-grid for belt, backpack, armory etc.
 */
void Place::drawgrid(BITMAP *dest, int PLACE_NUM)
{
    ASSERT((PLACE_NUM >= 0) && (PLACE_NUM <= NUMBER_OF_PLACES));

    if (PLACE_NUM == P_ARMOURY) {
        const char *eqname = get_current_equipment_name();
        if (!eqname) eqname = _("(none, remote player doesn't have any of your weaposets)");
        textprintf(dest, g_small_font, gx, gy + 1 - text_height(g_small_font), COLOR_WHITE,
            _("Weapon set: %s"), eqname);
    } else {
        textout(dest, g_small_font, place_name[PLACE_NUM], gx, gy + 1 - text_height(g_small_font), COLOR_LT_OLIVE);
    }

    if (!ishand()) {
        int dx = 0, dy = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (!outside_belt(j, i))
                    rect(dest, gx + dx, gy + dy, gx + dx + 16, gy + dy + 15, COLOR_GRAY08);      //square
                dx += 16;
                if (j == 19) break;      // for map cell!!!!!!!!!!!!!!!!!!!
            }
            dx = 0;
            dy += 15;
        }
    } else {
        rect(dest, gx, gy, gx + width * 16, gy + height * 15, COLOR_GRAY08);
    }

    // Draw items in grid:
    Item *t = m_item;
    while (t != NULL) {
        if ((t->m_x >= viscol - 1) && (t->m_x < viscol + 20)) {

            if (!is_item_allowed(t->itemtype())) {
                int x = gx + (t->m_x - viscol) * 16;
                int y = gy + t->m_y * 15;
                int sx = t->obdata_width() * 16;
                int sy = t->obdata_height() * 15;
                if (ishand()) {
                    x = gx;
                    y = gy;
                    sx = width * 16;
                    sy = height * 15;
                }
                rectfill(dest, x + 1, y + 1, x + sx - 1, y + sy - 1, COLOR_RED06);
            }

            int x = gx + (t->m_x - viscol) * 16;
            int y = gy + t->m_y * 15 + 5;

            if (ishand()) {
                int it_width = t->obdata_width();
                int it_height = t->obdata_height();
                x = gx + (width - it_width) * 16 / 2;
                y = gy + (height - it_height) * 15 / 2 + 5;
            }

            PCK::showpck(dest, t->obdata_pInv(), x, y);  // Picture of item

            if (ishand()) { // Inventory-view: display ammo-rounds & grenade-delay
                if (t->clip() != NULL) {   // see also: Soldier::drawinfo()
                    printsmall_x(dest, gx + 23, gy + 39, COLOR_WHITE, t->roundsremain() );
                    textout(dest, g_small_font, t->get_damage_name(), gx + 3, gy + 36, COLOR_GREEN);
                }
                if (t->obdata_isAmmo() ) {   // Test
                    printsmall_x(dest, gx + 23, gy + 39, COLOR_WHITE, t->m_rounds );
                }
                if (t->is_grenade() ) {  // see also: icon.h : DrawPrimed
                    if (t->delay_time() > 0)
                    printsmall_x(dest, gx + 23, gy + 39, COLOR_RED, t->delay_time() - 1);
                    else if (t->is_proximity_grenade() && t->delay_time() < 0)
                    textout(dest, g_small_font, "*", gx + 23, gy + 36, COLOR_RED);
                }
            }

            if (key[KEY_LCONTROL]) {
                t->draw_health(dest, 1, x + 1, y - 4);
            }
            if (key[KEY_ALT]) {
                t->draw_health(dest, 0, x + 1, y - 4);
            }
        }
        t = t->m_next;
    }
}

/**
 * Drop all items from place to given map position.
 * For example, a soldier panicking.
 */
void Place::dropall(int lev, int col, int row)
{
    Item *t = m_item;
    Position p;
    if (m_cell != NULL)
    	p = m_cell->get_position();
    while (t != NULL) {
        //text_mode(0);
        //textprintf(screen, font, 1, 150, 1, "x=%d y=%d t=%2x p=%4s n=%1s   ", t->x, t->y, t->type, t->prev, t->next);

        //Place::put modifies t, so store reference to next item beforehand
        Item* tm = t->m_next;
        if (m_cell != NULL)
            if (int ownlight = t->obdata_ownLight())	/* if item has own light - an electro flare */
				map->remove_light_source(p.level(), p.column(), p.row(), ownlight);
        map->place(lev, col, row)->put(t);

        //textprintf(screen, font, 1, 160, 1, "put t=%2x", t->type);
        //textprintf(screen, font, 1, 170, 1, "x=%d y=%d t=%2x p=%4s n=%1s   ", t->x, t->y, t->type, t->prev, t->next);
        t = tm;
    }
    m_item = NULL;
    if (m_cell != NULL)
        map->update_seen_item(p);

}

/**
 * Returns true if item is in this Place.
 */
int Place::isthere(Item *it)
{
    Item *t;
    t = m_item;
    while (t != NULL) {
        if (t == it) {
            ASSERT(it->m_place == this);
            return 1;
        }
        t = t->m_next;
    }
    ASSERT(it->m_place != this);
    return 0;
}

/**
 * Save current place in a lua format compatible with weaponset description
 */
void Place::export_as_weaponset(const char *fn)
{
    FILE *fh = fopen(F(fn), "wt");
    ASSERT(fh != NULL);

    fprintf(fh, "AddEquipment {\n");
    fprintf(fh, "    Name = \"%s\",\n", fn);
    fprintf(fh, "    Layout = {\n");
    Item *it = m_item;
    while (it != NULL) {
        if (it->m_place == this)
            fprintf(fh, "        {%02d, %02d, \"%s\"},\n", it->m_x, it->m_y, it->name().c_str());
        it = it->m_next;
    }
    fprintf(fh, "    }\n");
    fprintf(fh, "}\n");
    fclose(fh);
}

/**
 * Add item to a place (armoury, unit body parts, etc.)
 * @param x         x coordinate of an item inside of place
 * @param y         y coordinate of an item inside of place
 * @param item_name symbolic name of item
 * @param item_name symbolic name of item
 * @param autoload  boolean flag, if set to true, the game tries to automatically
 *                  load the weapon (if it does not have ammo choice)
 * @returns         success or failure
 */
bool Place::add_item(int x, int y, const char *item_name, bool autoload)
{
    Item *it = create_item(item_name);
    if (!it) return false;

    // Trying to put item here
    if (put(it, x, y)) {
        std::vector<std::string> ammo;
        Item::get_ammo_list(item_name, ammo);

        if (!autoload || ammo.size() != 1) return true;

        it = create_item(ammo[0].c_str());
        if (!it) return true;
    }

    // Maybe it is ammo for already added weapon?
    Item *weapon = get(x, y);
    if (weapon) {
        bool loaded = weapon->loadclip(it);
        put(weapon, x, y);
        if (loaded) return true;
    }

    // Nowhere to put it, giving up
    delete it;

    return false;
}

void Place::build_ITEMDATA(int ip, ITEMDATA * id) //don't save clip rounds
{
    Item * it = m_item;
    while (it != NULL)
    {
        ASSERT(id->num < 100);
        id->place[id->num] = ip;
        id->item_type[id->num] = intel_uint32(it->m_type);
        id->x[id->num] = it->m_x;
        id->y[id->num] = it->m_y;
        id->num++;
        if (it->haveclip()) {
            id->place[id->num] = 0xFF;
            id->item_type[id->num] = intel_uint32(it->cliptype());
            id->num++;
        }
        it = it->m_next;
    }
}

void Place::save_to_string(std::string &str)
{
    str.clear();

    // search for the last item in the list (it was added first)
    Item *it = m_item;
    while (it != NULL && it->m_next) it = it->m_next;

    // save items list in correct order (items added first are listed in the beginning of list)
    while (it != NULL) {
        char line[512];
        if (!it->haveclip()) {
            sprintf(line, "{%d, %d, \"%s\"},\n", it->m_x, it->m_y,
                lua_escape_string(it->name()).c_str());
        } else {
            sprintf(line, "{%d, %d, \"%s\", \"%s\"},\n", it->m_x, it->m_y,
                lua_escape_string(it->name()).c_str(),
                lua_escape_string(Item::obdata_name(it->cliptype())).c_str());
        }
        str += line;
        it = it->m_prev;
    }
}

void Place::build_items_stats(char *buf, int &len)
{
    Item *it = m_item;
    while (it != NULL) {
        buf[len++] = it->m_type;
        if (it->haveclip()) {
            buf[len++] = it->cliptype();
        }
        it = it->m_next;
    }
}

/**
 * Get and std::vector containing all items in this place.
 * @return Number of items.
 */
int Place::get_items_list(std::vector<Item *> &items)
{
    int count = 0;
    Item *it = m_item;
    while (it != NULL) {
        items.push_back(it);
        count++;
        if (it->haveclip()) {
            items.push_back(it->clip());
            count++;
        }
        it = it->m_next;
    }
    return count;
}

int Place::save_items(char *fs, int _z, int _x, int _y, char *txt)
{
    int len = 0;
    Item *t = m_item;

    char format[1000];
    sprintf(format, "%s type=%%d x=%%d y=%%d rounds=%%d\r\n", fs);

    while (t != NULL) {
        len += sprintf(txt + len, format, _z, _x, _y,
                       t->m_type, t->m_x, t->m_y, t->m_rounds);

        if (t->haveclip()) {
            len += sprintf(txt + len, format, _z, _x, _y,
                           t->cliptype(), -1, -1, t->roundsremain());
        }
        t = t->m_next;
    }
    return len;
}

/**
 * End-of-turn - Save
 */
int Place::eot_save(int ip, char *txt)
{
    int len = 0;
    Item *t = m_item;

    while (t != NULL) {
        len += sprintf(txt + len, "ip=%d type=%d x=%d y=%d rounds=%d\r\n",
                       ip, t->m_type, t->m_x, t->m_y, t->m_rounds);
        if (t->haveclip()) //////////////////////
        {
            len += sprintf(txt + len, "ip=%d type=%d x=%d y=%d rounds=%d\r\n",
                           -1, t->cliptype(), -1, -1, t->roundsremain());
        }
        t = t->m_next;
    }
    return len;
}

/**
 * Calculate and return weight for all the equipment stored at this place
 */
int Place::count_weight()
{
    Item *t = m_item;
    int weight = 0;

    while (t != NULL)
    {
        weight += t->obdata_weight();
        if (t->haveclip())
            weight += t->clip()->obdata_weight();
        t = t->m_next;
    }
    return weight;
}

/**
 * Check if any banned equipment (not in allowed equipment set)
 * is stored at this place
 */
int Place::has_forbidden_equipment()
{
    Item *t = m_item;

    while (t != NULL)
    {
        if (!is_item_allowed(t->itemtype()))
            return 1;
        t = t->m_next;
    }
    return 0;
}

/**
 * Destroy all items in place.
 */
void Place::destroy_all_items()
{
    Item *t, *t2;

    Position p;
    if (m_cell)	/* if this is a map cell, remember position */
    	p = m_cell->get_position();

    t = m_item;
    while (t != NULL) {
        t2 = t->m_next;
        if (m_cell != NULL) {
            if (int ownlight = t->obdata_ownLight())	/* if item has own light - an electro flare */
				map->remove_light_source(p.level(), p.column(), p.row(), ownlight);
        }
        t->unlink();
        delete t;
        t = t2;
    }
    m_item = NULL;
    if (m_cell != NULL)	/* if map cell, update view */
        map->update_seen_item(p);
}

/**
 * Damage items (for example an explosion hitting items on the ground)
 */
void Place::damage_items(int dam)
{
    Item *it = m_item;

	/* grenades explode by being damaged, so store them in a list when destroyed */
    std::vector<int> explo_type, explo_owner;

    while(it != NULL) {
        if (it->damage(dam)) { //destroyed
            Item *t2 = it->m_next;	// remember next as destroy() will delete this item

            if (it->is_grenade()) {
                explo_type.push_back(it->m_type);
                explo_owner.push_back(elist->get_owner(it));
            }

            elist->remove(it);
            destroy(it);

            it = t2;
            continue;	// jumped to next item so continue iteration
        }
        it = it->m_next;
    }

    if (explo_type.size() > 0) {
        int lev, col, row;
		
		/* Find the coordinates of this place. Can be found at 3 different places:
			- on the map
			- our soldier has it
			- the opponent's soldier has it */
        int pf = map->find_place_coords(this, lev, col, row);
        if (!pf)
            pf = platoon_local->find_place_coords(this, lev, col, row);
        if (!pf)
            pf = platoon_remote->find_place_coords(this, lev, col, row);
        ASSERT(pf);

        for (int i = 0; i < (signed)explo_type.size(); i++)
            map->explode(explo_owner[i], lev * 12, col * 16 + 8, row * 16 + 8, explo_type[i]);
    }
}

/**
 * Explodes proximity mine if this place has one
 */
bool Place::check_mine()
{
    Item *it = m_item;

    while (it != NULL) {
        if (it->is_grenade() && it->is_proximity_grenade() && (it->delay_time() < 0)) {
            elist->check_for_detonation(1, it);
            return true;
        }
        it = it->m_next;
    }
    return false;
}

/**
 * Show TUs needed to move an item to a place like hand, belt, backpack etc.
 */
void Place::draw_deselect_time(BITMAP *dest, int PLACE_NUM, int time)
{
    int color = COLOR_WHITE;
    if (time) {
        int ISLOCAL = platoon_local->belong(sel_man);
        if (sel_man->time_reserve(time, ISLOCAL, 0) != OK) color = COLOR_RED;
        printsmall_x(dest, gx + 1 + text_length(g_small_font, place_name[PLACE_NUM]), gy - 6, color, time);
    }
}

bool Place::Write(persist::Engine &archive) const
{
    PersistWriteBinary(archive, *this);

    PersistWriteObject(archive, m_item);
    PersistWriteObject(archive, m_cell);

    return true;
}

bool Place::Read(persist::Engine &archive)
{
    PersistReadBinary(archive, *this);

    PersistReadObject(archive, m_item);
    PersistReadObject(archive, m_cell);

    return true;
}

