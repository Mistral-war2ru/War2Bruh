#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "patch.h"//Fois patcher
#include "defs.h"//some addresses and constants
#include "names.h"//some constants

bool game_started = false;

WORD m_screen_w = 640;
WORD m_screen_h = 480;

byte a_custom = 0;//custom game
byte reg[4] = { 0 };//region
int* unit[1610];//array for units
int* unitt[1610];//temp array for units
int units = 0;//number of units in array
byte ua[255] = { 255 };//attackers array
byte ut[255] = { 255 };//targets array     only unit ua[i] can deal damage to unit ut[i]
bool first_step = false;//first step of trigger
bool ai_fixed = false;//ai fix
bool saveload_fixed = false;//saveload ai break bug fix
bool A_portal = false;//portals activated

struct Vizs
{
    byte x = 0;
    byte y = 0;
    byte p = 0;
    byte s = 0;
};
Vizs vizs_areas[2000];
int vizs_n = 0;

//-----sound
void* def_name = NULL;
void* def_sound = NULL;
void* def_name_seq = NULL;
void* def_sound_seq = NULL;

char bruh_name[] = "BRUH\\bruh.wav\x0";
void* bruh_sound = NULL;

void sound_play_from_file(int oid, DWORD name, void* snd)//old id
{
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * oid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * oid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)snd);
    ((void (*)(WORD))F_WAR2_SOUND_PLAY)(oid);//war2 sound play
    snd = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * oid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * oid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * oid), (DWORD)def_name);//restore default
}
//-----sound

void show_message(byte time, char* text)
{
    ((void (*)(char*, int, int))F_MAP_MSG)(text, 15, time * 10);//original war2 show msg func
}

int get_val(int adress, int player)
{
    return (int)(*(WORD*)(adress + player * 2));//player*2 cause all vals is WORD
}

bool cmp_args(byte m, byte v, byte c)
{//compare bytes
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

bool cmp_args2(byte m, WORD v, WORD c)
{//compare words
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

bool cmp_args4(byte m, int v, int c)
{//comapre 4 bytes (for resources)
    bool f = false;
    switch (m)
    {
    case CMP_EQ:f = (v == c); break;
    case CMP_NEQ:f = (v != c); break;
    case CMP_BIGGER_EQ:f = (v >= c); break;
    case CMP_SMALLER_EQ:f = (v <= c); break;
    case CMP_BIGGER:f = (v > c); break;
    case CMP_SMALLER:f = (v < c); break;
    default: f = false; break;
    }
    return f;
}

void lose(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\x0";//if need to show table
        PATCH_SET((char*)LOSE_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x3b";
        PATCH_SET((char*)LOSE_SHOW_TABLE, buf);
    }
	if (!first_step)
	{
		char l[] = "\x2";
        PATCH_SET((char*)(ENDGAME_STATE + (*(byte*)LOCAL_PLAYER)), l);
		((void (*)())F_LOSE)();//original lose func
	}
	else
	{
		patch_setdword((DWORD*)VICTORY_FUNCTION, (DWORD)F_LOSE);
	}
}

void win(bool t)
{
    game_started = false;
    if (t == true)
    {
        char buf[] = "\xEB";//if need to show table
        PATCH_SET((char*)WIN_SHOW_TABLE, buf);
    }
    else
    {
        char buf[] = "\x74";
        PATCH_SET((char*)WIN_SHOW_TABLE, buf);
    }
	if (!first_step)
	{
		char l[] = "\x3";
        PATCH_SET((char*)(ENDGAME_STATE + (*(byte*)LOCAL_PLAYER)), l);
		((void (*)())F_WIN)();//original win func
	}
	else
	{
		patch_setdword((DWORD*)VICTORY_FUNCTION, (DWORD)F_WIN);
	}
}

void tile_remove_trees(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_TREES)(x, y);
}

void tile_remove_rocks(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_ROCKS)(x, y);
}

void tile_remove_walls(int x, int y)
{
    ((void (*)(int, int))F_TILE_REMOVE_WALLS)(x, y);
}

bool stat_byte(byte s)
{//chech if unit stat is 1 or 2 byte
    bool f = (s == S_DRAW_X) || (s == S_DRAW_Y) || (s == S_X) || (s == S_Y) || (s == S_HP)
        || (s == S_INVIZ) || (s == S_SHIELD) || (s == S_BLOOD) || (s == S_HASTE)
        || (s == S_AI_SPELLS) || (s == S_NEXT_FIRE)
        || (s == S_LAST_HARVEST_X) || (s == S_LAST_HARVEST_Y)
        || (s == S_BUILD_PROGRES) || (s == S_BUILD_PROGRES_TOTAL)
        || (s == S_RESOURCES) || (s == S_ORDER_X) || (s == S_ORDER_Y)
        || (s == S_RETARGET_X1) || (s == S_RETARGET_Y1) || (s == S_RETARGET_X2) || (s == S_RETARGET_Y2);
    return !f;
}

bool cmp_stat(int* p, int v, byte pr, byte cmp)
{
    //p - unit
    //v - value
    //pr - property
    //cmp - compare method
    bool f = false;
    if (stat_byte(pr))
    {
        byte ob = v % 256;
        char buf[3] = { 0 };
        buf[0] = ob;
        buf[1] = *((byte*)((uintptr_t)p + pr));
        if (cmp_args(cmp, buf[1], buf[0]))
        {
            f = true;
        }
    }
    else
    {
        if (cmp_args2(cmp, *((WORD*)((uintptr_t)p + pr)), (WORD)v))
        {
            f = true;
        }
    }
    return f;
}

void set_stat(int* p, int v, byte pr)
{
    if (stat_byte(pr))
    {
        char buf[] = "\x0";
        buf[0] = v % 256;
        PATCH_SET((char*)((uintptr_t)p + pr), buf);
    }
    else
    {
        char buf[] = "\x0\x0";
        buf[0] = v % 256;
        buf[1] = (v / 256) % 256;
        PATCH_SET((char*)((uintptr_t)p + pr), buf);
    }
}

void unit_convert(byte player, int who, int tounit, int a)
{
    //original war2 func converts units
    ((void (*)(byte, int, int, int))F_UNIT_CONVERT)(player, who, tounit, a);
}

void unit_create(int x, int y, int id, byte owner, byte n)
{
    while (n > 0)
    {
        n -= 1;
        int* p = (int*)(UNIT_SIZE_TABLE + 4 * id);//unit sizes table
        ((void (*)(int, int, int, byte, int*))F_UNIT_CREATE)(x, y, id, owner, p);//original war2 func creates unit
        //just call n times to create n units
    }
}

void unit_kill(int* u)
{
    ((void (*)(int*))F_UNIT_KILL)(u);//original war2 func kills unit
}

void unit_remove(int* u)
{
    byte f = *((byte*)((uintptr_t)u + S_FLAGS3));
    f |= SF_HIDDEN;
    set_stat(u, f, S_FLAGS3);
    unit_kill(u);//hide unit then kill
}

void unit_cast(int* u)//unit autocast
{
    ((void (*)(int*))F_AI_CAST)(u);//original war2 ai cast spells
}

void bullet_create(WORD x, WORD y, byte id)
{
    int* b = ((int* (*)(WORD, WORD, byte))F_BULLET_CREATE)(x, y, id);
}

void bullet_create_unit(int* u, byte b)
{
    WORD x = *((WORD*)((uintptr_t)u + S_DRAW_X));
    WORD y = *((WORD*)((uintptr_t)u + S_DRAW_Y));
    bullet_create(x + 16, y + 16, b);
}

void bullet_create8_around_unit(int* u, byte b)
{
    WORD ux = *((WORD*)((uintptr_t)u + S_DRAW_X));
    WORD uy = *((WORD*)((uintptr_t)u + S_DRAW_Y));
    WORD x = ux + 16;
    WORD y = uy + 16;
    if ((b == B_LIGHT_FIRE) || (b == B_HEAVY_FIRE))y -= 8;
    if ((b == B_LIGHTNING) || (b == B_COIL))
    {
        x += 16;
        y += 16;
    }
    WORD xx = x;
    WORD yy = y;
    bullet_create(xx + 48, yy, b);//right
    bullet_create(xx, yy + 48, b);//down
    bullet_create(xx + 32, yy + 32, b);//right down
    if (xx <= 32)xx = 0;
    else xx -= 32;
    bullet_create(xx, yy + 32, b);//left down
    if (yy <= 32)yy = 0;
    else yy -= 32;
    bullet_create(xx, yy, b);//left up
    xx = x;
    bullet_create(xx + 32, yy, b);//right up
    yy = y;
    if (xx <= 48)xx = 0;
    else xx -= 48;
    bullet_create(xx, yy, b);//left
    xx = x;
    if (yy <= 48)yy = 0;
    else yy -= 48;
    bullet_create(xx, yy, b);//up
}

void set_region(int x1, int y1, int x2, int y2)
{
    if (x1 < 0)x1 = 0;
    if (x1 > 127)x1 = 127;
    if (y1 < 0)y1 = 0;
    if (y1 > 127)y1 = 127;
    if (x2 < 0)x2 = 0;
    if (x2 > 127)x2 = 127;
    if (y2 < 0)y2 = 0;
    if (y2 > 127)y2 = 127;
    reg[0] = x1 % 256;
    reg[1] = y1 % 256;
    reg[2] = x2 % 256;
    reg[3] = y2 % 256;
}

bool in_region(byte x, byte y, byte x1, byte y1, byte x2, byte y2)
{
    //dnt know why but without this big monstrous ussless code gam crash 
    byte tmp;
    tmp = x % 256;
    x = tmp;
    tmp = y % 256;
    y = tmp;
    tmp = x1 % 256;
    x1 = tmp;
    tmp = y1 % 256;
    y1 = tmp;
    tmp = x2 % 256;
    x2 = tmp;
    tmp = y2 % 256;
    y2 = tmp;
    if (x < 0)x = 0;
    if (x > 127)x = 127;
    if (y < 0)y = 0;
    if (y > 127)y = 127;
    if (x1 < 0)x1 = 0;
    if (x1 > 127)x1 = 127;
    if (y1 < 0)y1 = 0;
    if (y1 > 127)y1 = 127;
    if (x2 < 0)x2 = 0;
    if (x2 > 127)x2 = 127;
    if (y2 < 0)y2 = 0;
    if (y2 > 127)y2 = 127;
    if (x2 < x1)
    {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y2 < y1)
    {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    //just check if coords inside region
    return ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2));
}

bool check_unit_dead(int* p)
{
    bool dead = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3))
            & (SF_DEAD | SF_DIEING | SF_UNIT_FREE)) != 0)
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_unit_complete(int* p)//for buildings
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_COMPLETED) != 0)//flags3 last bit
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_hidden(int* p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_HIDDEN) != 0)//flags3 4 bit
            f = true;
    }
    else
        f = true;
    return f;
}

bool check_unit_preplaced(int* p)
{
    bool f = false;
    if (p)
    {
        if ((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_PREPLACED) != 0)//flags3
            f = true;
    }
    else
        f = false;
    return f;
}

bool check_unit_near_death(int* p)
{
    bool dead = false;
    if (p)
    {
        if (((*((byte*)((uintptr_t)p + S_FLAGS3)) & SF_DIEING) != 0)
            && ((*((byte*)((uintptr_t)p + S_FLAGS3)) & (SF_DEAD | SF_UNIT_FREE)) == 0))
            dead = true;
    }
    else
        dead = true;
    return dead;
}

bool check_peon_loaded(int* p, byte r)
{
    bool f = false;
    if (p)
    {
        if (r == 0)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0)
                && ((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_GOLD) != 0))
                f = true;
        }
        if (r == 1)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0)
                && ((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_HARVEST_LUMBER) != 0))
                f = true;
        }
        if (r == 2)
        {
            if (((*((byte*)((uintptr_t)p + S_PEON_FLAGS)) & PEON_LOADED) != 0))
                f = true;
        }
    }
    return f;
}

void find_all_units(byte id)
{
	//CAREFUL with this function - ALL units get into massive 
    //even if their memory was cleared already
    //all units by id will go in array
    units = 0;
    int* p = (int*)UNITS_MASSIVE;//pointer to units
    p = (int*)(*p);
    int k = *(int*)UNITS_NUMBER;
    while (k > 0)
    {
        bool f = *((byte*)((uintptr_t)p + S_ID)) == (byte)id;
        if (f)
        {
            unit[units] = p;
            units++;
        }
        p = (int*)((int)p + 0x98);
        k--;
    }
}

void find_all_alive_units(byte id)
{
    //all units by id will go in array
    units = 0;
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);//pointer to units list for each player
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = *((byte*)((uintptr_t)p + S_ID)) == (byte)id;
                if (id == ANY_BUILDING)
                    f = *((byte*)((uintptr_t)p + S_ID)) >= U_FARM;//buildings
                if (id == ANY_MEN)
                    f = *((byte*)((uintptr_t)p + S_ID)) < U_FARM;//all nonbuildings
                if (id == ANY_UNITS)
                    f = true;//all ALL units
                if (id == ANY_BUILDING_2x2)//small buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 2;
                }
                if (id == ANY_BUILDING_3x3)//med buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 3;
                }
                if (id == ANY_BUILDING_4x4)//big buildings
                {
                    byte sz = *((byte*)UNIT_SIZE_TABLE + *((byte*)((uintptr_t)p + S_ID)) * 4);
                    f = sz == 4;
                }
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        unit[units] = p;
                        units++;
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void sort_complete()
{
    //only completed units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_complete(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_in_region()
{
    //only units in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD*)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD*)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_not_in_region()
{
    //only units not in region stay in array
    int k = 0;
    WORD x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((WORD*)((uintptr_t)unit[i] + S_DRAW_X)) / 32;
        y = *((WORD*)((uintptr_t)unit[i] + S_DRAW_Y)) / 32;
        if (!in_region((byte)x, (byte)y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_target_in_region()
{
    //only units that have order coords in region stay in array
    int k = 0;
    byte x = 0, y = 0;
    for (int i = 0; i < units; i++)
    {
        x = *((byte*)((uintptr_t)unit[i] + S_ORDER_X));
        y = *((byte*)((uintptr_t)unit[i] + S_ORDER_Y));
        if (in_region(x, y, reg[0], reg[1], reg[2], reg[3]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_stat(byte pr, int v, byte cmp)
{
    //only units stay in array if have property compared to value is true
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (cmp_stat(unit[i], v, pr, cmp))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_hidden()
{
    //only not hidden units stay in array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_hidden(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_self(int* u)
{
    //unit remove self from array
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!(unit[i] == u))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_preplaced()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_unit_preplaced(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_near_death()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_unit_near_death(unit[i]))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit(int* p)
{
    //only units stay in array that *p can attack them
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int(*)(int*, int*))F_ATTACK_CAN_HIT)(p, unit[i]);//attack can hit original war2 function
        if (a != 0)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_attack_can_hit_range(int* p)
{
    //only units stay in array that *p can attack them and have passable terrain in attack range
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        int a = 0;
        a = ((int(*)(int*, int*))F_ATTACK_CAN_HIT)(p, unit[i]);//attack can hit
        if (a != 0)
        {
            byte id = *((byte*)((uintptr_t)unit[i] + S_ID));
            byte szx = *(byte*)(UNIT_SIZE_TABLE + 4 * id);
            byte szy = *(byte*)(UNIT_SIZE_TABLE + 4 * id + 2);
            byte idd = *((byte*)((uintptr_t)p + S_ID));
            byte rng = *(byte*)(UNIT_RANGE_TABLE + idd);
            byte ms = *(byte*)MAP_SIZE;
            byte xx = *((byte*)((uintptr_t)unit[i] + S_X));
            byte yy = *((byte*)((uintptr_t)unit[i] + S_Y));
            if (xx < rng)xx = 0;
            else xx -= rng;
            if (yy < rng)yy = 0;
            else yy -= rng;
            byte cl = *((byte*)((uintptr_t)p + S_MOVEMENT_TYPE));//movement type
            WORD mt = *(WORD*)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl);//movement terrain flags

            bool f = false;
            for (int x = xx; (x < szx + xx + rng * 2 + 1) && (x < 127); x++)
            {
                for (int y = yy; (y < szy + yy + rng * 2 + 1) && (x < 127); y++)
                {
                    int aa = 1;
                    if ((cl == 0) || (cl == 3))//land and docked transport
                    {
                        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);//original war2 func if terrain passable with that movement type
                    }
                    if ((x % 2 == 0) && (y % 2 == 0))//air and water
                    {
                        if ((cl == 1) || (cl == 2))
                        {
                            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
                        }
                    }
                    if (aa == 0)f = true;
                }
            }
            if (f)
            {
                unitt[k] = unit[i];
                k++;
            }
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_rune_near()
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte*)((uintptr_t)unit[i] + S_X));
        byte y = *((byte*)((uintptr_t)unit[i] + S_Y));
        bool f = false;
        for (int r = 0; r < 50; r++)//max runes 50
        {
            WORD d = *(WORD*)(RUNEMAP_TIMERS + 2 * r);
            if (d != 0)
            {
                byte xx = *(byte*)(RUNEMAP_X + r);
                byte yy = *(byte*)(RUNEMAP_Y + r);
                if (xx == x)
                {
                    if (yy > y)
                    {
                        if ((yy - y) == 1)f = true;
                    }
                    else
                    {
                        if ((y - yy) == 1)f = true;
                    }
                }
                if (yy == y)
                {
                    if (xx > x)
                    {
                        if ((xx - x) == 1)f = true;
                    }
                    else
                    {
                        if ((x - xx) == 1)f = true;
                    }
                }
            }
        }
        if (!f)
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void sort_peon_not_loaded(byte r)
{
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if (!check_peon_loaded(unit[i], r))
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void set_stat_all(byte pr, int v)
{
    for (int i = 0; i < units; i++)
    {
        set_stat(unit[i], v, pr);//set stat to all units in array
    }
}

void kill_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_kill(unit[i]);//just kill all in array
    }
    units = 0;
}

void remove_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_remove(unit[i]);//just kill all in array
    }
    units = 0;
}

void cast_all()
{
    for (int i = 0; i < units; i++)
    {
        unit_cast(unit[i]);//casting spells
    }
    units = 0;
}

void damag(int* p, byte n1, byte n2)
{
    WORD hp = *((WORD*)((uintptr_t)p + S_HP));//unit hp
    WORD n = n1 + 256 * n2;
    if (hp > n)
    {
        hp -= n;
        set_stat(p, hp, S_HP);
    }
    else
    {
        set_stat(p, 0, S_HP);
        unit_kill(p);
    }
}

void damag_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        damag(unit[i], n1, n2);
    }
}

void heal(int* p, byte n1, byte n2)
{
    byte id = *((byte*)((uintptr_t)p + S_ID));//unit id
    WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * id);//max hp
    WORD hp = *((WORD*)((uintptr_t)p + S_HP));//unit hp
    WORD n = n1 + 256 * n2;
    if (hp < mhp)
    {
        hp += n;
        if (hp > mhp)
            hp = mhp;//canot heal more than max hp
        set_stat(p, hp, S_HP);
    }
}

void heal_all(byte n1, byte n2)
{
    for (int i = 0; i < units; i++)
    {
        heal(unit[i], n1, n2);
    }
}

void peon_load(int* u, byte r)
{
    byte f = *((byte*)((uintptr_t)u + S_PEON_FLAGS));
    if (!(f & PEON_LOADED))
    {
        if (r == 0)
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_GOLD;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int*))F_GROUP_SET)(u);
        }
        else
        {
            f |= PEON_LOADED;
            f |= PEON_HARVEST_LUMBER;
            set_stat(u, f, S_PEON_FLAGS);
            ((void (*)(int*))F_GROUP_SET)(u);
        }
    }
}

void peon_load_all(byte r)
{
    for (int i = 0; i < units; i++)
    {
        peon_load(unit[i], r);
    }
}

void viz_area(byte x, byte y, byte pl, byte sz)
{
    int Vf = F_VISION2;
    switch (sz)
    {
    case 0:Vf = F_VISION2; break;
    case 1:Vf = F_VISION2; break;
    case 2:Vf = F_VISION2; break;
    case 3:Vf = F_VISION3; break;
    case 4:Vf = F_VISION4; break;
    case 5:Vf = F_VISION5; break;
    case 6:Vf = F_VISION6; break;
    case 7:Vf = F_VISION7; break;
    case 8:Vf = F_VISION8; break;
    case 9:Vf = F_VISION9; break;
    default: Vf = F_VISION2; break;
    }
    for (byte i = 0; i < 8; i++)
    {
        if (((1 << i) & pl) != 0)
        {
            ((void (*)(WORD, WORD, byte))Vf)(x, y, i);
        }
    }
}

void viz_area_add(byte x, byte y, byte pl, byte sz)
{
    if ((vizs_n >= 0) && (vizs_n <= 255))
    {
        vizs_areas[vizs_n].x = x;
        vizs_areas[vizs_n].y = y;
        vizs_areas[vizs_n].p = pl;
        vizs_areas[vizs_n].s = sz;
        vizs_n++;
    }
}

void viz_area_all(byte pl, byte sz)
{
    for (int i = 0; i < units; i++)
    {
        byte x = *((byte*)((uintptr_t)unit[i] + S_X));
        byte y = *((byte*)((uintptr_t)unit[i] + S_Y));
        viz_area_add(x, y, pl, sz);
    }
}

void give(int* p, byte owner)
{
    ((void (*)(int*, byte, byte))F_CAPTURE)(p, owner, 1);//original capture unit war2 func
    *(byte*)(RESCUED_UNITS + 2 * owner) -= 1;//reset number of captured units
}

void give_all(byte o)
{
    for (int i = 0; i < units; i++)
    {
        give(unit[i], o);
    }
}

bool unit_move(byte x, byte y, int* unit)
{
    if (x < 0)return false;
    if (y < 0)return false;//canot go negative
    byte mxs = *(byte*)MAP_SIZE;//map size
    if (x >= mxs)return false;
    if (y >= mxs)return false;//canot go outside map
    if (check_unit_hidden(unit))return false;//if unit not hidden
    byte cl = *((byte*)((uintptr_t)unit + S_MOVEMENT_TYPE));//movement type
    WORD mt = *(WORD*)(GLOBAL_MOVEMENT_TERRAIN_FLAGS + 2 * cl);//movement terrain flags

    int aa = 1;
    if ((cl == 0) || (cl == 3))//land and docked transport
    {
        aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);//original war2 func if terrain passable with that movement type
    }
    if ((x % 2 == 0) && (y % 2 == 0))//air and water
    {
        if ((cl == 1) || (cl == 2))
        {
            aa = ((int (*)(int, int, int))F_XY_PASSABLE)(x, y, (int)mt);
        }
    }
    if (aa == 0)
    {
        ((void (*)(int*))F_UNIT_UNPLACE)(unit);//unplace
        set_stat(unit, x, S_X);
        set_stat(unit, y, S_Y);//change real coords
        set_stat(unit, x * 32, S_DRAW_X);
        set_stat(unit, y * 32, S_DRAW_Y);//change draw sprite coords
        ((void (*)(int*))F_UNIT_PLACE)(unit);//place
        return true;
    }
    return false;
}

void move_all(byte x, byte y)
{
    sort_stat(S_ID, U_FARM, CMP_SMALLER);//non buildings
    sort_stat(S_ANIMATION, 2, CMP_EQ);//only if animation stop
    for (int i = 0; i < units; i++)
    {
        int xx = 0, yy = 0, k = 1;
        bool f = unit_move(x, y, unit[i]);
        xx--;
        while ((!f) & (k < 5))//goes in spiral like original war2 (size 5)
        {
            while ((!f) & (yy < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy++;
            }
            while ((!f) & (xx < k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx++;
            }
            while ((!f) & (yy > -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                yy--;
            }
            while ((!f) & (xx >= -k))
            {
                f = unit_move(x + xx, y + yy, unit[i]);
                xx--;
            }
            k++;
        }
    }
}

void give_order(int* u, byte x, byte y, byte o)
{
    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id < U_FARM)
    {
        char buf[] = "\x0";
        bool f = ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT));
        if (f)
        {
            buf[0] = o;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }
        int* tr = NULL;
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);//pointer to units list for each player
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    if (p!=u)
                    {
                        if (!check_unit_dead(p) && !check_unit_hidden(p))
                        {
                            byte xx = *(byte*)((uintptr_t)p + S_X);
                            byte yy = *(byte*)((uintptr_t)p + S_Y);
                            if ((abs(x - xx) <= 2) && (abs(y - yy) <= 2))
                            {
                                if (f)
                                {
                                    byte idd = *(byte*)((uintptr_t)p + S_ID);
                                    if (idd < U_FARM)
                                    {
                                        bool trf = true;
                                        if (o == ORDER_SPELL_ARMOR)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_SHIELD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_BLOODLUST)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_BLOOD);
                                            trf = ef == 0;
                                        }
                                        if (o == ORDER_SPELL_HASTE)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef > 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_SLOW)
                                        {
                                            WORD ef = *(WORD*)((uintptr_t)p + S_HASTE);
                                            trf = (ef == 0) || (ef <= 0x7FFF);
                                        }
                                        if (o == ORDER_SPELL_POLYMORPH)
                                        {
                                            trf = idd != U_CRITTER;
                                        }
                                        if (o == ORDER_SPELL_HEAL)
                                        {
                                            WORD mhp = *(WORD*)(UNIT_HP_TABLE + 2 * idd);
                                            WORD hp = *((WORD*)((uintptr_t)p + S_HP));
                                            trf = hp < mhp;
                                        }
                                        if (trf)
                                        {
                                            WORD efi = *(WORD*)((uintptr_t)p + S_INVIZ);
                                            trf = efi == 0;
                                        }
                                        if (trf)
                                            tr = p;
                                    }
                                }
                                else
                                    tr = p;
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
        bool aoe = (o == ORDER_SPELL_VISION) || (o == ORDER_SPELL_EXORCISM) || (o == ORDER_SPELL_FIREBALL) ||
            (o == ORDER_SPELL_BLIZZARD) || (o == ORDER_SPELL_EYE) || (o == ORDER_SPELL_RAISEDEAD) ||
            (o == ORDER_SPELL_DRAINLIFE) || (o == ORDER_SPELL_WHIRLWIND) || (o == ORDER_SPELL_RUNES) ||
            (o == ORDER_SPELL_ROT) || (o == ORDER_MOVE) || (o == ORDER_PATROL) ||
            (o == ORDER_ATTACK_AREA) || (o == ORDER_ATTACK_WALL) || (o == ORDER_STAND) ||
            (o == ORDER_ATTACK_GROUND) || (o == ORDER_ATTACK_GROUND_MOVE) || (o == ORDER_DEMOLISH) ||
            (o == ORDER_HARVEST) || (o == ORDER_RETURN) || (o == ORDER_UNLOAD_ALL) || (o == ORDER_STOP);

        if (o != ORDER_ATTACK_WALL)
        {
            int ord = *(int*)(ORDER_FUNCTIONS + 4 * o);//orders functions
            if (!aoe && (tr != NULL) && (tr != u))
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, tr, ord);//original war2 order
            if (aoe)
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, x, y, NULL, ord);//original war2 order
        }
        else
        {
            byte oru = *(byte*)((uintptr_t)u + S_ORDER);
            if (oru!=ORDER_ATTACK_WALL)
            {
                int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_STOP);//orders functions
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, NULL, ord);//original war2 order
            }
            set_stat(u, ORDER_ATTACK_WALL, S_NEXT_ORDER);
            set_stat(u, x, S_ORDER_X);
            set_stat(u, y, S_ORDER_Y);
        }

        if (f)
        {
            buf[0] = 0;
            PATCH_SET((char*)GW_ACTION_TYPE, buf);
        }
    }
}

void give_order_spell_target(int* u, int* t, byte o)
{
    if ((u != NULL) && (t != NULL))
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if (id < U_FARM)
        {
            char buf[] = "\x0";
            if ((o >= ORDER_SPELL_VISION) && (o <= ORDER_SPELL_ROT))
            {
                buf[0] = o;
                PATCH_SET((char*)GW_ACTION_TYPE, buf);

                int ord = *(int*)(ORDER_FUNCTIONS + 4 * o);//orders functions
                ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(u, 0, 0, t, ord);//original war2 order

                buf[0] = 0;
                PATCH_SET((char*)GW_ACTION_TYPE, buf);
            }
        }
    }
}

void order_all(byte x, byte y, byte o)
{
    for (int i = 0; i < units; i++)
    {
        give_order(unit[i], x, y, o);
    }
}

bool check_ally(byte p1, byte p2)
{
    //check allied table
    return ((*(byte*)(ALLY + p1 + 16 * p2) != 0) && (*(byte*)(ALLY + p2 + 16 * p1) != 0));
}

bool slot_alive(byte p)
{
    return (get_val(ALL_BUILDINGS, p) + (get_val(ALL_UNITS, p) - get_val(FLYER, p))) > 0;//no units and buildings
}

void ally(byte p1, byte p2, byte a)
{
    //set ally bytes in table
    *(byte*)(ALLY + p1 + 16 * p2) = a;
    *(byte*)(ALLY + p2 + 16 * p1) = a;
    ((void (*)())F_RESET_COLORS)();//orig war2 func reset colors of sqares around units
}

void ally_one_sided(byte p1, byte p2, byte a)
{
    //set ally bytes in table
    *(byte*)(ALLY + p1 + 16 * p2) = a;
    ((void (*)())F_RESET_COLORS)();//orig war2 func reset colors of sqares around units
}

bool check_opponents(byte player)
{
    //check if player have opponents
    bool f = false;
    byte o = C_NOBODY;
    for (byte i = 0; i < 8; i++)
    {
        if (player != i)
        {
            if (slot_alive(i) && !check_ally(player, i))//if enemy and not dead
                f = true;
        }
    }
    return f;
}

void viz(int p1, int p2, byte a)
{
    //set vision bits
    byte v = *(byte*)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte*)(VIZ + p1) = v;

    v = *(byte*)(VIZ + p2);
    if (a == 0)
        v = v & (~(1 << p1));
    else
        v = v | (1 << p1);
    *(byte*)(VIZ + p2) = v;
}

void viz_one_sided(int p1, int p2, byte a)
{
    //set vision bits
    byte v = *(byte*)(VIZ + p1);
    if (a == 0)
        v = v & (~(1 << p2));
    else
        v = v | (1 << p2);
    *(byte*)(VIZ + p1) = v;
}

void comps_vision(bool v)
{
    //comps can give vision too
    if (v)
    {
        char o[] = "\x0";
        PATCH_SET((char*)COMPS_VIZION, o);
        char o2[] = "\x90\x90";
        PATCH_SET((char*)COMPS_VIZION2, o2);
        char o3[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char*)COMPS_VIZION3, o3);
    }
    else
    {
        char o[] = "\xAA";
        PATCH_SET((char*)COMPS_VIZION, o);
        char o2[] = "\x84\xC9";
        PATCH_SET((char*)COMPS_VIZION2, o2);
        char o3[] = "\xF\x85\x8C\x0\x0\x0";
        PATCH_SET((char*)COMPS_VIZION3, o3);
    }
}

void change_res(byte p, byte r, byte k, int m)
{
    int a = GOLD;
    int* rs = (int*)a;
    DWORD res = 0;
    bool s = false;
    if (p >= 0 && p <= 8)//player id
    {
        switch (r)//select resource and add or substract it
        {
        case 0:
            a = GOLD + 4 * p;
            s = false;
            break;
        case 1:
            a = LUMBER + 4 * p;
            s = false;
            break;
        case 2:
            a = OIL + 4 * p;
            s = false;
            break;
        case 3:
            a = GOLD + 4 * p;
            s = true;
            break;
        case 4:
            a = LUMBER + 4 * p;
            s = true;
            break;
        case 5:
            a = OIL + 4 * p;
            s = true;
            break;
        default:break;
        }
        if (r >= 0 && r <= 5)
        {
            rs = (int*)a;//resourse pointer
            if (s)
            {
                if (*rs > (int)(k * m))
                    res = *rs - (k * m);
                else
                    res = 0;//canot go smaller than 0
            }
            else
            {
                if (*rs <= (256 * 256 * 256 * 32))
                    res = *rs + (k * m);
            }
            patch_setdword((DWORD*)a, res);
        }
    }
}

void add_total_res(byte p, byte r, byte k, int m)
{
    int a = GOLD_TOTAL;
    int* rs = (int*)a;
    DWORD res = 0;
    if (p >= 0 && p <= 8)//player id
    {
        switch (r)//select resource and add or substract it
        {
        case 0:
            a = GOLD_TOTAL + 4 * p;
            break;
        case 1:
            a = LUMBER_TOTAL + 4 * p;
            break;
        case 2:
            a = OIL_TOTAL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int*)a;//resourse pointer
            if (*rs <= (256 * 256 * 256 * 32))
                res = *rs + (k * m);
            patch_setdword((DWORD*)a, res);
        }
    }
}

void set_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4)
{
    //as before but dnt add or sub res, just set given value
    char buf[4] = { 0 };
    int a = 0;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            buf[0] = k1;
            buf[1] = k2;
            buf[2] = k3;
            buf[3] = k4;
            PATCH_SET((char*)a, buf);
        }
    }
}

bool cmp_res(byte p, byte r, byte k1, byte k2, byte k3, byte k4, byte cmp)
{
    //compare resource to value
    int a = GOLD;
    int* rs = (int*)a;
    if (p >= 0 && p <= 8)
    {
        switch (r)
        {
        case 0:
            a = GOLD + 4 * p;
            break;
        case 1:
            a = LUMBER + 4 * p;
            break;
        case 2:
            a = OIL + 4 * p;
            break;
        default:break;
        }
        if (r >= 0 && r <= 2)
        {
            rs = (int*)a;
            return cmp_args4(cmp, *rs, k1 + 256 * k2 + 256 * 256 * k3 + 256 * 256 * 256 * k4);
        }
    }
    return false;
}

int empty_false(byte) { return 0; }//always return false function
int empty_true(byte) { return 1; }//always return true function
void empty_build(int id)
{
    ((void (*)(int))F_TRAIN_UNIT)(id);//original build unit func
}
void empty_build_building(int id)
{
    ((void (*)(int))F_BUILD_BUILDING)(id);//original build func
}
void empty_build_research(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH)(id);
}
void empty_build_research_spell(int id)
{
    ((void (*)(int))F_BUILD_RESEARCH_SPELL)(id);
}
void empty_build_upgrade_self(int id)
{
    ((void (*)(int))F_BUILD_UPGRADE_SELF)(id);
}
void empty_cast_spell(int id)
{
    ((void (*)(int))F_CAST_SPELL)(id);
}

void empty_upgrade_th1(int id)
{
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    set_stat(u, 0, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}
void empty_upgrade_th2(int id)
{
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    set_stat(u, 1, S_AI_AIFLAGS);
    empty_build_upgrade_self(id);
}

int empty_research_swords(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SWORDS)(id); }//0 or 1
int empty_research_shield(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIELD)(id); }//0 or 1
int empty_research_cat(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_CAT)(id); }//0 or 1
int empty_research_arrows(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_ARROWS)(id); }//0 or 1
int empty_research_ships_at(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_AT)(id); }//0 or 1
int empty_research_ships_def(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SHIPS_DEF)(id); }//0 or 1
int empty_research_ranger(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_RANGER)(id); }
int empty_research_scout(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SCOUT)(id); }
int empty_research_long(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_LONG)(id); }
int empty_research_marks(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_MARKS)(id); }
int empty_research_spells(byte id) { return ((int (*)(int))F_CHECK_RESEARCH_SPELL)(id); }
//00444410
int empty_upgrade_th(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TH)(id); }//0 or 1
int empty_upgrade_tower(byte id) { return ((int (*)(int))F_CHECK_UPGRADE_TOWER)(id); }//0 or 1
int empty_spell_learned(byte id) { return ((int (*)(int))F_CHECK_SPELL_LEARNED)(id); }

int _2tir() 
{ 
    if ((get_val(TH2, *(byte*)LOCAL_PLAYER) != 0) || (get_val(TH3, *(byte*)LOCAL_PLAYER) != 0))
        return 1;
    else
        return 0;
}

int _3tir()
{
    if (get_val(TH3, *(byte*)LOCAL_PLAYER) != 0)
        return 1;
    else
        return 0;
}

int get_marks()
{
    if (*(byte*)(GB_MARKS + *(byte*)LOCAL_PLAYER))
        return 1;
    else
        return 0;
}

void repair_cat(bool b)
{
    //peon can repair unit if it have transport flag OR catapult flag
    if (b)
    {
        char r1[] = "\xeb\x75\x90\x90\x90";//f6 c4 04 74 14
        PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
        char r2[] = "\x66\xa9\x04\x04\x74\x9c\xeb\x86";
        PATCH_SET((char*)REPAIR_CODE_CAVE, r2);
    }
    else
    {
        char r1[] = "\xf6\xc4\x4\x74\x14";
        PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
    }
}

void trigger_time(byte tm)
{
    //war2 will call victory check function every 200 game ticks
    char ttime[] = "\xc8";//200 default
    ttime[0] = tm;
    PATCH_SET((char*)TRIG_TIME, ttime);
}

void manacost(byte id, byte c)
{
    //spells cost of mana
    char mana[] = "\x1";
    mana[0] = c;
    PATCH_SET((char*)(MANACOST + 2 * id), mana);
}

void upgr(byte id, byte c)
{
    //upgrades power
    char up[] = "\x1";
    up[0] = c;
    PATCH_SET((char*)(UPGRD + id), up);
}

byte get_upgrade(byte id, byte pl)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:a = GB_ARROWS; break;
    case SWORDS:a = GB_SWORDS; break;
    case ARMOR:a = GB_SHIELDS; break;
    case SHIP_DMG:a = GB_BOAT_ATTACK; break;
    case SHIP_ARMOR:a = GB_BOAT_ARMOR; break;
    case SHIP_SPEED:a = GB_BOAT_SPEED; break;
    case CATA_DMG:a = GB_CAT_DMG; break;
    default:a = GB_ARROWS; break;
    }
    return *(byte*)(a + pl);
}

void set_upgrade(byte id, byte pl, byte v)
{
    int a = GB_ARROWS;
    switch (id)
    {
    case ARROWS:a = GB_ARROWS; break;
    case SWORDS:a = GB_SWORDS; break;
    case ARMOR:a = GB_SHIELDS; break;
    case SHIP_DMG:a = GB_BOAT_ATTACK; break;
    case SHIP_ARMOR:a = GB_BOAT_ARMOR; break;
    case SHIP_SPEED:a = GB_BOAT_SPEED; break;
    case CATA_DMG:a = GB_CAT_DMG; break;
    default:a = GB_ARROWS; break;
    }
    char buf[] = "\x0";
    buf[0] = v;
    PATCH_SET((char*)(a + pl), buf);
    ((void (*)())F_STATUS_REDRAW)();//status redraw
}

int upgr_check_swords(byte b)
{
    byte u = get_upgrade(SWORDS, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_shields(byte b)
{
    byte u = get_upgrade(ARMOR, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_boat_attack(byte b)
{
    byte u = get_upgrade(SHIP_DMG, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_boat_armor(byte b)
{
    byte u = get_upgrade(SHIP_ARMOR, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

int upgr_check_arrows(byte b)
{
    byte u = get_upgrade(ARROWS, *(byte*)LOCAL_PLAYER);
    if ((b == 0) && (u == 0))return 1;
    if ((b == 1) && (u == 1))return 1;
    if ((b == 2) && (u >= 2))return 1;
    return 0;
}

void upgr_check_replace(bool f)
{
    if (f)
    {
        char buf[] = "\xC3";//ret
        patch_ljmp((char*)UPGR_CHECK_FIX_SWORDS, (char*)upgr_check_swords);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SWORDS + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIELDS, (char*)upgr_check_shields);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIELDS + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIPS_AT, (char*)upgr_check_boat_attack);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIPS_AT + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_SHIPS_DEF, (char*)upgr_check_boat_armor);
        PATCH_SET((char*)(UPGR_CHECK_FIX_SHIPS_DEF + 5), buf);
        patch_ljmp((char*)UPGR_CHECK_FIX_ARROWS, (char*)upgr_check_arrows);
        PATCH_SET((char*)(UPGR_CHECK_FIX_ARROWS + 5), buf);
    }
    else
    {
        char buf2[] = "\x33\xC0\x33\xC9\xA0\x18";//back
        PATCH_SET((char*)UPGR_CHECK_FIX_SWORDS, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIELDS, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIPS_AT, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_SHIPS_DEF, buf2);
        PATCH_SET((char*)UPGR_CHECK_FIX_ARROWS, buf2);
    }
}

void center_view(byte x, byte y)
{
    ((void (*)(byte, byte))F_MINIMAP_CLICK)(x, y);//original war2 func that called when player click on minimap
}

PROC g_proc_00451054;
void count_add_to_tables_load_game(int* u)
{
    if (saveload_fixed)
    {
        byte f = *((byte*)((uintptr_t)u + S_AI_AIFLAGS));
        byte ff = f | AI_PASSIVE;
        set_stat(u, ff, S_AI_AIFLAGS);
        ((void (*)(int*))g_proc_00451054)(u);//original
        set_stat(u, f, S_AI_AIFLAGS);
    }
    else
        ((void (*)(int*))g_proc_00451054)(u);//original
}

PROC g_proc_00438A5C;
PROC g_proc_00438985;
void unset_peon_ai_flags(int* u)
{
    ((void (*)(int*))g_proc_00438A5C)(u);//original
    if (saveload_fixed)
    {
        char rep[] = "\x0\x0";
        WORD p = 0;
        for (int i = 0; i < 8; i++)
        {
            p = *((WORD*)((uintptr_t)SGW_REPAIR_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_REPAIR_PEONS + 2 * i), rep);
            p = *((WORD*)((uintptr_t)SGW_GOLD_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_GOLD_PEONS + 2 * i), rep);
            p = *((WORD*)((uintptr_t)SGW_TREE_PEONS + 2 * i));
            if (p > 1600)
                PATCH_SET((char*)(SGW_TREE_PEONS + 2 * i), rep);
        }
    }
}

void tech_built(int p, byte t)
{
    ((void (*)(int, byte))F_TECH_BUILT)(p, t);
}

void tech_reinit()
{
    for (int i = 0; i < 8; i++)
    {
        byte o = *(byte*)(CONTROLER_TYPE + i);
        byte a = 0;
        int s = 0;
        if (o == C_COMP)
        {
            a = *(byte*)(GB_ARROWS + i);
            if (a > 0)tech_built(i, UP_ARROW1);
            if (a > 1)tech_built(i, UP_ARROW2);
            a = *(byte*)(GB_SWORDS + i);
            if (a > 0)tech_built(i, UP_SWORD1);
            if (a > 1)tech_built(i, UP_SWORD2);
            a = *(byte*)(GB_SHIELDS + i);
            if (a > 0)tech_built(i, UP_SHIELD1);
            if (a > 1)tech_built(i, UP_SHIELD2);
            a = *(byte*)(GB_BOAT_ATTACK + i);
            if (a > 0)tech_built(i, UP_BOATATK1);
            if (a > 1)tech_built(i, UP_BOATATK2);
            a = *(byte*)(GB_BOAT_ARMOR + i);
            if (a > 0)tech_built(i, UP_BOATARM1);
            if (a > 1)tech_built(i, UP_BOATARM2);
            a = *(byte*)(GB_CAT_DMG + i);
            if (a > 0)tech_built(i, UP_CATDMG1);
            if (a > 1)tech_built(i, UP_CATDMG2);
            a = *(byte*)(GB_RANGER + i);
            if (a)tech_built(i, UP_RANGER);
            a = *(byte*)(GB_MARKS + i);
            if (a)tech_built(i, UP_SKILL1);
            a = *(byte*)(GB_LONGBOW + i);
            if (a)tech_built(i, UP_SKILL2);
            a = *(byte*)(GB_SCOUTING + i);
            if (a)tech_built(i, UP_SKILL3);

            s = *(int*)(SPELLS_LEARNED + 4 * i);
            if (s & (1 << L_ALTAR_UPGR))tech_built(i, UP_CLERIC);
            if (s & (1 << L_HEAL))tech_built(i, UP_CLERIC1);
            if (s & (1 << L_BLOOD))tech_built(i, UP_CLERIC1);
            if (s & (1 << L_EXORCISM))tech_built(i, UP_CLERIC2);
            if (s & (1 << L_RUNES))tech_built(i, UP_CLERIC2);
            if (s & (1 << L_FLAME_SHIELD))tech_built(i, UP_WIZARD1);
            if (s & (1 << L_RAISE))tech_built(i, UP_WIZARD1);
            if (s & (1 << L_SLOW))tech_built(i, UP_WIZARD2);
            if (s & (1 << L_HASTE))tech_built(i, UP_WIZARD2);
            if (s & (1 << L_INVIS))tech_built(i, UP_WIZARD3);
            if (s & (1 << L_WIND))tech_built(i, UP_WIZARD3);
            if (s & (1 << L_POLYMORF))tech_built(i, UP_WIZARD4);
            if (s & (1 << L_UNHOLY))tech_built(i, UP_WIZARD4);
            if (s & (1 << L_BLIZZARD))tech_built(i, UP_WIZARD5);
            if (s & (1 << L_DD))tech_built(i, UP_WIZARD5);

            find_all_alive_units(U_KEEP);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)tech_built(i, UP_KEEP);
            find_all_alive_units(U_STRONGHOLD);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)tech_built(i, UP_KEEP);
            find_all_alive_units(U_CASTLE);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
            find_all_alive_units(U_FORTRESS);
            sort_stat(S_OWNER, i, CMP_EQ);
            if (units != 0)
            {
                tech_built(i, UP_KEEP);
                tech_built(i, UP_CASTLE);
            }
        }
    }
}

void building_start_build(int* u, byte id, byte o)
{
    ((void (*)(int*, byte, byte))F_BLDG_START_BUILD)(u, id, o);
}

void build_inventor(int* u)
{
    if (check_unit_complete(u))
    {
        byte f = *((byte*)((uintptr_t)u + S_FLAGS1));
        if (!(f & UF_BUILD_ON))
        {
            byte id = *((byte*)((uintptr_t)u + S_ID));
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            int spr = get_val(ACTIVE_SAPPERS, o);
            byte nspr = *(byte*)(AIP_NEED_SAP + 48 * o);
            if (nspr > spr)
            {
                if (id == U_INVENTOR)building_start_build(u, U_DWARWES, 0);
                if (id == U_ALCHEMIST)building_start_build(u, U_GOBLINS, 0);
            }
            int flr = get_val(ACTIVE_FLYER, o);
            byte nflr = *(byte*)(AIP_NEED_FLYER + 48 * o);
            if (nflr > flr)
            {
                if (id == U_INVENTOR)building_start_build(u, U_FLYER, 0);
                if (id == U_ALCHEMIST)building_start_build(u, U_ZEPPELIN, 0);
            }
        }
    }
}

void build_sap_fix(bool f)
{
    if (f)
    {
        char b1[] = "\x80\xfa\x40\x0";
        void (*r1) (int*) = build_inventor;
        patch_setdword((DWORD*)b1, (DWORD)r1);
        PATCH_SET((char*)BLDG_WAIT_INVENTOR, b1);//human inv
        PATCH_SET((char*)(BLDG_WAIT_INVENTOR + 4), b1);//orc inv
    }
    else
    {
        char b1[] = "\x80\xfa\x40\x0";
        PATCH_SET((char*)BLDG_WAIT_INVENTOR, b1);//human inv
        PATCH_SET((char*)(BLDG_WAIT_INVENTOR + 4), b1);//orc inv
    }
}

void ai_fix_plugin(bool f)
{
    if (f)
    {
        char b1[] = "\xb2\x02";
        PATCH_SET((char*)AIFIX_PEONS_REP, b1);//2 peon rep
        char b21[] = "\xbb\x8";
        PATCH_SET((char*)AIFIX_GOLD_LUMB1, b21);//gold lumber
        char b22[] = "\xb4\x4";
        PATCH_SET((char*)AIFIX_GOLD_LUMB2, b22);//gold lumber
        char b3[] = "\x1";
        PATCH_SET((char*)AIFIX_BUILD_SIZE, b3);//packed build
        char b4[] = "\xbe\x0\x0\x0\x0\x90\x90";
        PATCH_SET((char*)AIFIX_FIND_HOME, b4);//th corner
        char b6[] = "\x90\x90\x90\x90\x90\x90";
        PATCH_SET((char*)AIFIX_POWERBUILD, b6);//powerbuild
        char m7[] = "\x90\x90";
        PATCH_SET((char*)AIFIX_RUNES_INV, m7);//runes no inviz
        ai_fixed = true;
        build_sap_fix(true);
    }
    else
    {
        char b1[] = "\x8a\xd0";
        PATCH_SET((char*)AIFIX_PEONS_REP, b1);//2 peon rep
        char b21[] = "\xd0\x7";
        PATCH_SET((char*)AIFIX_GOLD_LUMB1, b21);//gold lumber
        char b22[] = "\xf4\x1";
        PATCH_SET((char*)AIFIX_GOLD_LUMB2, b22);//gold lumber
        char b3[] = "\x6";
        PATCH_SET((char*)AIFIX_BUILD_SIZE, b3);//packed build
        char b4[] = "\xe8\xf8\x2a\x1\x0\x8b\xf0";
        PATCH_SET((char*)AIFIX_FIND_HOME, b4);//th corner
        char b6[] = "\xf\x84\x78\x1\x0\x0";
        PATCH_SET((char*)AIFIX_POWERBUILD, b6);//powerbuild
        char m7[] = "\x74\x1d";
        PATCH_SET((char*)AIFIX_RUNES_INV, m7);//runes no inviz
        ai_fixed = false;
        build_sap_fix(false);
    }
}

PROC g_proc_0040EEDD;
void upgrade_tower(int* u, int id, int b)
{
    if (ai_fixed)
    {
        bool c = false;
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if ((get_val(LUMBERMILL, o) == 0) && (get_val(SMITH, o) != 0)) c = true;
        if ((get_val(LUMBERMILL, o) != 0) && (get_val(SMITH, o) != 0) && ((get_val(TOWER, o) % 2) == 0)) c = true;
        if (c)id += 2;
    }
    ((void (*)(int*, int, int))g_proc_0040EEDD)(u, id, b);//original
}

PROC g_proc_00442E25;
void create_skeleton(int x, int y, int id, int o)
{
    if (ai_fixed)
    {
        unit_create((x / 32) + 1, y / 32, id, o % 256, 1);
    }
    else
        ((void (*)(int, int, int, int))g_proc_00442E25)(x, y, id, o);//original
}

PROC g_proc_00425D1C;
int* cast_raise(int* u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        find_all_alive_units(U_SKELETON);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_preplaced();
        if (units < 10)
        {
            if (((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_RAISE)) == 0))return NULL;
            byte mp = *((byte*)((uintptr_t)u + S_MANA));
            byte cost = *(byte*)(MANACOST + 2 * RAISE_DEAD);
            if (mp < cost)return NULL;
            byte x = *((byte*)((uintptr_t)u + S_X));
            byte y = *((byte*)((uintptr_t)u + S_Y));
            set_region((int)x - 8, (int)y - 8, (int)x + 8, (int)y + 8);//set region around myself
            find_all_units(ANY_BUILDING);//dead body
            sort_in_region();
            sort_hidden();
            sort_near_death();
            if (units != 0)
            {
                byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
                byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
                give_order(u, xx, yy, ORDER_SPELL_RAISEDEAD);
                return unit[0];
            }
        }
        return NULL;
    }
    else
        return ((int* (*)(int*, int, int, int))g_proc_00425D1C)(u, a1, a2, a3);//original
}

PROC g_proc_00424F94;
PROC g_proc_00424FD7;
int* cast_runes(int* u, int a1, int a2, int a3)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        if (((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_RUNES)) == 0))return NULL;
        byte mp = *((byte*)((uintptr_t)u + S_MANA));
        byte cost = *(byte*)(MANACOST + 2 * RUNES);
        if (mp < cost)return NULL;
        byte x = *((byte*)((uintptr_t)u + S_X));
        byte y = *((byte*)((uintptr_t)u + S_Y));
        set_region((int)x - 14, (int)y - 14, (int)x + 14, (int)y + 14);//set region around myself
        find_all_alive_units(ANY_MEN);
        sort_in_region();
        sort_hidden();
        sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
        for (int ui = 0; ui < 16; ui++)
        {
            if (check_ally(o, ui))//only not allied units
                sort_stat(S_OWNER, ui, CMP_NEQ);
        }
        sort_rune_near();
        if (units != 0)
        {
            byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
            byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
            give_order(u, xx, yy, ORDER_SPELL_RUNES);
            return unit[0];
        }
        return NULL;
    }
    else
        return ((int* (*)(int*, int, int, int))g_proc_00424F94)(u, a1, a2, a3);//original
}

byte get_manacost(byte s)
{
    return *(byte*)(MANACOST + 2 * s);
}

PROC g_proc_0042757E;
int ai_spell(int* u)
{
    if (ai_fixed)
    {
        byte id = *((byte*)((uintptr_t)u + S_ID));
        if ((id == U_MAGE) || (id == U_DK))
        {
            byte x = *((byte*)((uintptr_t)u + S_X));
            byte y = *((byte*)((uintptr_t)u + S_Y));
            set_region((int)x - 24, (int)y - 24, (int)x + 24, (int)y + 24);//set region around myself
            find_all_alive_units(ANY_UNITS);
            sort_in_region();
            byte o = *((byte*)((uintptr_t)u + S_OWNER));
            for (int ui = 0; ui < 16; ui++)
            {
                if (check_ally(o, ui))
                    sort_stat(S_OWNER, ui, CMP_NEQ);
            }
            if (units != 0)
            {
                byte mp = *((byte*)((uintptr_t)u + S_MANA));
                byte ord = *((byte*)((uintptr_t)u + S_ORDER));
                bool new_cast = (ord == ORDER_SPELL_ROT) || (ord == ORDER_SPELL_BLIZZARD) || (ord == ORDER_SPELL_INVIS) || (ord == ORDER_SPELL_ARMOR);
                WORD shl = *((WORD*)((uintptr_t)u + S_SHIELD));
                WORD inv = *((WORD*)((uintptr_t)u + S_INVIZ));
                if ((shl == 0) && (inv == 0))
                {
                    set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui))//enemy
                            sort_stat(S_OWNER, ui, CMP_NEQ);
                    }
                    if (units != 0)
                    {
                        struct GPOINT
                        {
                            WORD x;
                            WORD y;
                        } l;
                        l.x = *((WORD*)((uintptr_t)u + S_X));
                        l.y = *((WORD*)((uintptr_t)u + S_Y));
                        ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(u, AI_ORDER_DEFEND, &l);
                        set_stat(u, l.x, S_AI_DEST_X);
                        set_stat(u, l.y, S_AI_DEST_Y);
                    }
                    if (mp < 50)new_cast = false;
                    else
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_INVIS)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(INVIS))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_INVIZ, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_INVIS);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_POLYMORF)) != 0)
                            {
                                if (ord != ORDER_SPELL_POLYMORPH)
                                {
                                    if (mp >= get_manacost(POLYMORPH))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui))//enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_POLYMORPH);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                        else if (id == U_DK)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_UNHOLY)) != 0)
                            {
                                if (ord != ORDER_SPELL_ARMOR)
                                {
                                    if (mp >= get_manacost(UNHOLY_ARMOR))
                                    {
                                        set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_EQ);
                                        sort_stat(S_MANA, 150, CMP_BIGGER_EQ);
                                        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_ARMOR);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else new_cast = true;
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_HASTE)) != 0)
                            {
                                if (ord != ORDER_SPELL_HASTE)
                                {
                                    if (mp >= get_manacost(HASTE))
                                    {
                                        set_region((int)x - 16, (int)y - 16, (int)x + 16, (int)y + 16);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DK, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_MAGE, CMP_BIGGER_EQ);
                                        sort_self(u);
                                        sort_stat(S_SHIELD, 0, CMP_NEQ);
                                        sort_stat(S_HASTE, 0, CMP_EQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (!check_ally(o, ui))
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            give_order_spell_target(u, unit[0], ORDER_SPELL_HASTE);
                                            new_cast = true;
                                        }
                                    }
                                }
                                else new_cast = true;
                            }
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_COIL)) != 0)
                            {
                                if (ord != ORDER_SPELL_DRAINLIFE)
                                {
                                    if (mp >= get_manacost(COIL))
                                    {
                                        set_region((int)x - 18, (int)y - 18, (int)x + 18, (int)y + 18);//set region around myself
                                        find_all_alive_units(ANY_MEN);
                                        sort_in_region();
                                        sort_stat(S_ID, U_DRAGON, CMP_SMALLER_EQ);
                                        sort_stat(S_ID, U_GRIFON, CMP_BIGGER_EQ);
                                        sort_stat(S_ANIMATION, ANIM_MOVE, CMP_NEQ);
                                        for (int ui = 0; ui < 16; ui++)
                                        {
                                            if (check_ally(o, ui))//enemy
                                                sort_stat(S_OWNER, ui, CMP_NEQ);
                                        }
                                        if (units != 0)
                                        {
                                            byte xx = *((byte*)((uintptr_t)unit[0] + S_X));
                                            byte yy = *((byte*)((uintptr_t)unit[0] + S_Y));
                                            give_order(u, xx, yy, ORDER_SPELL_DRAINLIFE);
                                            new_cast = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    set_region((int)x - 50, (int)y - 50, (int)x + 50, (int)y + 50);//set region around myself
                    find_all_alive_units(ANY_MEN);
                    sort_in_region();
                    sort_stat(S_ID, U_PEON, CMP_SMALLER_EQ);
                    sort_stat(S_ID, U_PEASANT, CMP_BIGGER_EQ);
                    sort_peon_loaded(0);
                    sort_hidden();
                    for (int ui = 0; ui < 16; ui++)
                    {
                        if (check_ally(o, ui))
                            sort_stat(S_OWNER, ui, CMP_NEQ);//enemy peons
                    }
                    if (units == 0)
                    {
                        find_all_alive_units(ANY_BUILDING);
                        sort_in_region();
                        sort_stat(S_ID, U_FORTRESS, CMP_SMALLER_EQ);//fortres castle stronghold keep
                        sort_stat(S_ID, U_KEEP, CMP_BIGGER_EQ);
                        for (int ui = 0; ui < 16; ui++)
                        {
                            if (check_ally(o, ui))
                                sort_stat(S_OWNER, ui, CMP_NEQ);
                        }
                        if (units == 0)
                        {
                            find_all_alive_units(ANY_BUILDING);
                            sort_in_region();
                            sort_stat(S_ID, U_GREAT_HALL, CMP_SMALLER_EQ);
                            sort_stat(S_ID, U_TOWN_HALL, CMP_BIGGER_EQ);
                            for (int ui = 0; ui < 16; ui++)
                            {
                                if (check_ally(o, ui))
                                    sort_stat(S_OWNER, ui, CMP_NEQ);
                            }
                        }
                    }
                    if (units != 0)
                    {
                        if (id == U_MAGE)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_BLIZZARD)) != 0)
                            {
                                if (ord != ORDER_SPELL_BLIZZARD)
                                {
                                    if (mp >= get_manacost(BLIZZARD))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_BLIZZARD);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                        }
                        if (id == U_DK)
                        {
                            if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_DD)) != 0)
                            {
                                if (ord != ORDER_SPELL_ROT)
                                {
                                    if (mp >= get_manacost(DEATH_AND_DECAY))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_ROT);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                            else if ((*(DWORD*)(SPELLS_LEARNED + 4 * o) & (1 << L_WIND)) != 0)
                            {
                                if (!new_cast && (ord != ORDER_SPELL_WHIRLWIND))
                                {
                                    if (mp >= get_manacost(WHIRLWIND))
                                    {
                                        byte tx = *((byte*)((uintptr_t)unit[0] + S_X));
                                        byte ty = *((byte*)((uintptr_t)unit[0] + S_Y));
                                        give_order(u, tx, ty, ORDER_SPELL_WHIRLWIND);
                                        new_cast = true;
                                    }
                                }
                                else new_cast = true;
                            }
                        }
                    }
                }
                if (!new_cast)
                    return ((int (*)(int*))g_proc_0042757E)(u);//original
            }
        }
        else
            return ((int (*)(int*))g_proc_0042757E)(u);//original
        return 0;
    }
    else
        return ((int (*)(int*))g_proc_0042757E)(u);//original
}

PROC g_proc_00427FAE;
void ai_attack(int* u, int b, int a)
{
    if (ai_fixed)
    {
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_MAGE) || (*((byte*)((uintptr_t)p + S_ID)) == U_DK));
                    if (f)
                    {
                        if (!check_unit_dead(p) && !check_unit_hidden(p))
                        {
                            byte ow = *((byte*)((uintptr_t)p + S_OWNER));
                            if (ow == o)
                            {
                                if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                                {
                                    WORD inv = *((WORD*)((uintptr_t)p + S_INVIZ));
                                    if (inv == 0)
                                    {
                                        byte aor = *((byte*)((uintptr_t)p + S_AI_ORDER));
                                        if (aor != AI_ORDER_ATTACK)
                                        {
                                            byte mp = *((byte*)((uintptr_t)p + S_MANA));
                                            if (mp >= 200)
                                            {
                                                ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(p, AI_ORDER_ATTACK, a);//ai attack
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }

        find_all_alive_units(ANY_MEN);
        sort_stat(S_ID, U_GOBLINS, CMP_SMALLER_EQ);
        sort_stat(S_ID, U_DWARWES, CMP_BIGGER_EQ);
        sort_stat(S_OWNER, o, CMP_EQ);
        sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_NEQ);//not attack already
        for (int i = 0; i < units; i++)
        {
            ((void (*)(int*, int, int))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, a);//ai attack
        }
    }
    ((void (*)(int*, int, int))g_proc_00427FAE)(u, b, a);//original
}

PROC g_proc_00427FFF;
void ai_attack_nearest_enemy(int* u, WORD x, WORD y, int* t, int ordr)
{
    if (ai_fixed)
    {
        struct GPOINT
        {
            WORD x;
            WORD y;
        };
        GPOINT l;
        l.x = *((WORD*)((uintptr_t)t + S_X));
        l.y = *((WORD*)((uintptr_t)t + S_Y));
        ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(u, AI_ORDER_ATTACK, &l);
    }
    else ((void (*)(int*, WORD, WORD, int*, int))g_proc_00427FFF)(u, x, y, t, ordr);//original
}

void sap_behaviour()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_DWARWES) || (*((byte*)((uintptr_t)p + S_ID)) == U_GOBLINS));
                if (f)
                {
                    if (!check_unit_dead(p) && !check_unit_hidden(p))
                    {
                        byte o = *((byte*)((uintptr_t)p + S_OWNER));
                        if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                        {
                            byte ord = *((byte*)((uintptr_t)p + S_ORDER));
                            byte x = *((byte*)((uintptr_t)p + S_X));
                            byte y = *((byte*)((uintptr_t)p + S_Y));
                            if ((ord != ORDER_DEMOLISH) && (ord != ORDER_DEMOLISH_NEAR) && (ord != ORDER_DEMOLISH_AT))
                            {
                                set_region((int)x - 12, (int)y - 12, (int)x + 12, (int)y + 12);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 5, (int)y - 5, (int)x + 5, (int)y + 5);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                                set_region((int)x - 1, (int)y - 1, (int)x + 1, (int)y + 1);//set region around myself
                                find_all_alive_units(ANY_UNITS);
                                sort_in_region();
                                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                                for (int ui = 0; ui < 16; ui++)
                                {
                                    if (check_ally(o, ui))//only not allied units
                                        sort_stat(S_OWNER, ui, CMP_NEQ);
                                }
                                if (units != 0)
                                {
                                    int ord = *(int*)(ORDER_FUNCTIONS + 4 * ORDER_DEMOLISH);
                                    ((void (*)(int*, int, int, int*, int))F_GIVE_ORDER)(p, 0, 0, unit[0], ord);
                                }
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void unstuk()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                byte id = *((byte*)((uintptr_t)p + S_ID));
                byte mov = *((byte*)((uintptr_t)p + S_MOVEMENT_TYPE));
                byte ord = *((byte*)((uintptr_t)p + S_ORDER));
                byte aord = *((byte*)((uintptr_t)p + S_AI_ORDER));
                bool f = ((((id < U_CRITTER) && (mov == MOV_LAND))
                    && !check_unit_preplaced(p) && ((ord == ORDER_STOP) || (ord == ORDER_MOVE_PATROL)) && (aord == AI_ORDER_ATTACK)) ||
                    ((id == U_PEASANT) || (id == U_PEON)));
                //if (*(byte*)GB_MULTIPLAYER)if (!((id == U_PEASANT) || (id == U_PEON)))f = false;
                if (f)
                {
                    if (!check_unit_dead(p))
                    {
                        if (!check_unit_hidden(p))
                        {
                            byte o = *((byte*)((uintptr_t)p + S_OWNER));
                            if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                            {
                                byte st = *((byte*)((uintptr_t)p + S_NEXT_FIRE));
                                byte frm = *((byte*)((uintptr_t)p + S_FRAME));
                                byte pfrm = *((byte*)((uintptr_t)p + S_NEXT_FIRE + 1));
                                if (st == 0)
                                {
                                    byte map = *(byte*)MAP_SIZE - 1;
                                    byte x = *((byte*)((uintptr_t)p + S_X));
                                    byte y = *((byte*)((uintptr_t)p + S_Y));
                                    int xx = 0, yy = 0, dir = 0;
                                    xx += x;
                                    yy += y;
                                    dir = ((int (*)())F_NET_RANDOM)();
                                    dir %= 8;
                                    if (dir == 0)
                                    {
                                        if (yy > 0)yy -= 1;
                                    }
                                    if (dir == 1)
                                    {
                                        if (yy > 0)yy -= 1;
                                        if (xx < map)xx += 1;
                                    }
                                    if (dir == 2)
                                    {
                                        if (xx < map)xx += 1;
                                    }
                                    if (dir == 3)
                                    {
                                        if (xx < map)xx += 1;
                                        if (yy < map)yy += 1;
                                    }
                                    if (dir == 4)
                                    {
                                        if (yy < map)yy += 1;
                                    }
                                    if (dir == 5)
                                    {
                                        if (yy < map)yy += 1;
                                        if (xx > 0)xx -= 1;
                                    }
                                    if (dir == 6)
                                    {
                                        if (xx > 0)xx -= 1;
                                    }
                                    if (dir == 7)
                                    {
                                        if (xx > 0)xx -= 1;
                                        if (yy > 0)yy -= 1;
                                    }
                                    give_order(p, xx % 256, yy % 256, ORDER_MOVE);
                                    st = 255;
                                }
                                if (st > 0)st -= 1;
                                if (frm != pfrm)st = 255;
                                set_stat(p, st, S_NEXT_FIRE);
                                set_stat(p, frm, S_NEXT_FIRE + 1);
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void goldmine_ai()
{
    for (int i = 0; i < 16; i++)
    {
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            while (p)
            {
                bool f = ((*((byte*)((uintptr_t)p + S_ID)) == U_MINE));
                if (f)
                {
                    if (check_unit_complete(p))
                    {
                        byte x = *((byte*)((uintptr_t)p + S_X));
                        byte y = *((byte*)((uintptr_t)p + S_Y));
                        set_region((int)x - 9, (int)y - 9, (int)x + 8, (int)y + 8);
                        find_all_alive_units(ANY_BUILDING_4x4);
                        sort_in_region();
                        sort_stat(S_ID, U_PORTAL, CMP_NEQ);
                        bool th = units != 0;
                        byte x1, y1, x2, y2;
                        if (x > 3)x1 = x - 3;
                        else x1 = 0;
                        if (y > 3)y1 = y - 3;
                        else y1 = 0;
                        x += 3;
                        y += 3;
                        if (x >= (127 - 3))x2 = 127;
                        else x2 = x + 3;
                        if (y >= (127 - 3))y2 = 127;
                        else y2 = y + 3;
                        char* sq = (char*)*(int*)(MAP_SQ_POINTER);
                        byte mxs = *(byte*)MAP_SIZE;//map size
                        for (int xx = x1; xx < x2; xx++)
                        {
                            for (int yy = y1; yy < y2; yy++)
                            {
                                char buf[] = "\x0";
                                buf[0] = *(char*)(sq + 2 * xx + 2 * yy * mxs + 1);
                                if (th)buf[0] |= SQ_AI_BUILDING >> 8;
                                else buf[0] &= ~(SQ_AI_BUILDING >> 8);
                                PATCH_SET((char*)(sq + 2 * xx + 2 * yy * mxs + 1), buf);
                            }
                        }
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
        }
    }
}

void set_tp_flag(bool f)
{
    for (int i = 0; i < units; i++)
    {
        //set if unit can be teleported by portal (that flag unused in actual game)
        if (f)
            *((byte*)((uintptr_t)unit[i] + S_FLAGS3)) |= SF_TELEPORT;
        else
            *((byte*)((uintptr_t)unit[i] + S_FLAGS3)) &= ~SF_TELEPORT;
    }
}

void sort_tp_flag()
{
    //if not teleported by portal
    int k = 0;
    for (int i = 0; i < units; i++)
    {
        if ((*((byte*)((uintptr_t)unit[i] + S_FLAGS3)) & SF_TELEPORT) == 0)//unused in actual game flag
        {
            unitt[k] = unit[i];
            k++;
        }
    }
    units = k;
    for (int i = 0; i < units; i++)
    {
        unit[i] = unitt[i];
    }
}

void portal()
{
    bool mp = true;
    for (int i = 0; i < 16; i++)
    {
        units = 0;
        int* p = (int*)(UNITS_LISTS + 4 * i);
        if (p)
        {
            p = (int*)(*p);
            int* fp = NULL;
            while (p)
            {
                bool f = *((byte*)((uintptr_t)p + S_ID)) == U_PORTAL;
                if (f)
                {
                    if (!check_unit_dead(p) && check_unit_complete(p))
                    {//alive and completed portal
                        if (!fp)fp = p;//remember first portal
                        byte tx = *((byte*)((uintptr_t)p + S_X)) + 1;
                        byte ty = *((byte*)((uintptr_t)p + S_Y)) + 1;//exit point is in center of portal
                        move_all(tx, ty);//teleport from previous portal
                        set_tp_flag(true);
                        set_stat_all(S_NEXT_ORDER, ORDER_STOP);
                        set_stat_all(S_ORDER_X, 128);
                        set_stat_all(S_ORDER_Y, 128);
                        byte x = *((byte*)((uintptr_t)p + S_X));
                        byte y = *((byte*)((uintptr_t)p + S_Y));
                        set_region((int)x - 1, (int)y - 1, (int)x + 4, (int)y + 4);//set region around myself
                        find_all_alive_units(ANY_MEN);
                        sort_in_region();
                        sort_tp_flag();//flag show if unit was not teleported
                        sort_stat(S_ORDER, ORDER_STOP, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 1, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 2, 0, CMP_EQ);
                        sort_stat(S_ORDER_UNIT_POINTER + 3, 0, CMP_EQ);
                        set_region((int)x, (int)y, (int)x + 3, (int)y + 3);//set region inside myself
                        sort_target_in_region();
                    }
                }
                p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
            }
            if (fp)//first portal teleports from last
            {
                byte tx = *((byte*)((uintptr_t)fp + S_X)) + 1;
                byte ty = *((byte*)((uintptr_t)fp + S_Y)) + 1;
                move_all(tx, ty);
                set_tp_flag(true);
                set_stat_all(S_NEXT_ORDER, ORDER_STOP);
                set_stat_all(S_ORDER_X, 128);
                set_stat_all(S_ORDER_Y, 128);
            }
        }
    }
    find_all_alive_units(ANY_MEN);
    set_tp_flag(false);//reset tp flags to all
}

PROC g_proc_0045271B;
void update_spells()
{
    ((void (*)())g_proc_0045271B)();//original
    // this function called every game tick
    // you can place your self-writed functions here in the end if need

    if (A_portal)portal();

	if (saveload_fixed)tech_reinit();
    if (ai_fixed)
    {
		unstuk();
		goldmine_ai();
        sap_behaviour();
    }
	if (vizs_n > 0)
    {
        for (int i = 0; i < vizs_n; i++)
        {
            viz_area(vizs_areas[i].x, vizs_areas[i].y, vizs_areas[i].p, vizs_areas[i].s);
        }
    }

    ((void (*)(int, int, int, int))F_INVALIDATE)(0, 0, m_screen_w, m_screen_h);//screen redraw
}

void seq_change(int* u, byte tt)
{
    byte t = tt;
    if (t == 1)
    {
        byte t = *((byte*)((uintptr_t)u + S_ANIMATION_TIMER));
        byte id = *((byte*)((uintptr_t)u + S_ID));
        byte a = *((byte*)((uintptr_t)u + S_ANIMATION));
        byte f = *((byte*)((uintptr_t)u + S_FRAME));
        byte o = *((byte*)((uintptr_t)u + S_OWNER));
        //if (id == U_ARCHER)
        //{
        //    if (a == ANIM_ATTACK)
        //    {
        //        if (f != 0)
        //        {
        //            if (get_upgrade(ARROWS, o) == 1)
        //                if (t > 9)t -= 2;
        //            if (get_upgrade(ARROWS, o) == 2)
        //                if (t > 9)t -= 4;
        //        }
        //        else
        //        {
        //            if (get_upgrade(ARROWS, o) == 1)
        //                if (t > 40)t -= 7;
        //            if (get_upgrade(ARROWS, o) == 2)
        //                if (t > 40)t -= 14;
        //        }
        //        set_stat(u, 1, S_ATTACK_COUNTER);
        //    }
        //    set_stat(u, t, S_ANIMATION_TIMER);
        //}
    }
}

PROC g_proc_004522B9;
int seq_run(int* u)
{
    byte t = *((byte*)((uintptr_t)u + S_ANIMATION_TIMER));
    bool f = true;
    int original = 0;
    if (f)
        original = ((int (*)(int*))g_proc_004522B9)(u);//original
    seq_change(u, t);
    return original;
}

PROC g_proc_00409F3B, //action
g_proc_0040AF70, g_proc_0040AF99, //demolish
g_proc_0041038E, g_proc_00410762, //bullet
g_proc_004428AD;//spell
char fdmg = 0;//final damage
void damage(int* atk, int* trg, char dmg)
{
    fdmg = dmg;
    if ((trg != NULL) && (atk != NULL))
    {
        if (!check_unit_dead(trg))
        {
            byte aid = *((byte*)((uintptr_t)atk + S_ID));//attacker id
            byte tid = *((byte*)((uintptr_t)trg + S_ID));//target id
            byte dmg2 = dmg;//can deal damage by default
            int i = 0;
            while (i < 255)
            {
                if ((tid == ut[i]) && (aid != ua[i]))
                {
                    dmg2 = 0;//canot deal damage
                }
                if ((tid == ut[i]) && (aid == ua[i]))//check if only some certain units can attack that unit
                {
                    dmg2 = dmg;//can deal damage
                    i = 255;
                }
                i++;
                if (ua[i] == 255)//pairs must go in a row
                {
                    i = 255;
                }
            }
            if (*((WORD*)((uintptr_t)trg + S_SHIELD)) != 0)dmg2 = 0;
            fdmg = dmg2;
            WORD hp = *((WORD*)((uintptr_t)trg + S_HP));//unit hp
            if ((tid < U_FARM) && (fdmg >= hp))
            {
                if ((aid != U_HTRANSPORT) || (aid != U_OTRANSPORT))
                {
                    byte k = *((byte*)((uintptr_t)atk + S_KILLS));
                    if (k < 255)k += 1;
                    set_stat(atk, (int)k, S_KILLS);
                }
            }
        }
    }
}

void damage1(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_00409F3B)(atk, trg, fdmg);
}

void damage2(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0041038E)(atk, trg, fdmg);
}

void damage3(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0040AF70)(atk, trg, fdmg);
}

void damage4(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_0040AF99)(atk, trg, fdmg);
}

void damage5(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_00410762)(atk, trg, fdmg);
}

void damage6(int* atk, int* trg, char dmg)
{
    damage(atk, trg, dmg);
    ((void (*)(int*, int*, char))g_proc_004428AD)(atk, trg, fdmg);
}

PROC g_proc_0040DF71;
int* bld_unit_create(int a1,int a2,int a3,byte a4,int* a5)
{
    int *b = (int*)*(int*)UNIT_RUN_UNIT_POINTER;
    int* u = ((int* (*)(int, int, int, byte, int*))g_proc_0040DF71)(a1, a2, a3, a4, a5);
    if (b)
    {
        if (u)
        {
            if (ai_fixed)
            {
                byte o = *((byte*)((uintptr_t)u + S_OWNER));
                byte m = *((byte*)((uintptr_t)u + S_MANA));
                if ((*(byte*)(CONTROLER_TYPE + o) == C_COMP))
                {
                    if (m = 0x55)//85 default starting mana
                    {
                        char buf[] = "\xA0";//160
                        PATCH_SET((char*)u + S_MANA, buf);
                    }
                }
            }
        }
    }
    return u;
}

PROC g_proc_00451728;
void unit_kill_deselect(int* u)
{
    int* ud = u;
    ((void (*)(int*))g_proc_00451728)(u);//original
    if (ai_fixed)
    {
        for (int i = 0; i < 16; i++)
        {
            int* p = (int*)(UNITS_LISTS + 4 * i);
            if (p)
            {
                p = (int*)(*p);
                while (p)
                {
                    byte id = *((byte*)((uintptr_t)p + S_ID));
                    bool f2 = ((id == U_DWARWES) || (id == U_GOBLINS));
                    if (f2)
                    {
                        if (!check_unit_dead(p))
                        {
                            byte a1 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER));
                            byte a2 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 1));
                            byte a3 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 2));
                            byte a4 = *((byte*)((uintptr_t)p + S_ORDER_UNIT_POINTER + 3));
                            int* tr = (int*)(a1 + 256 * a2 + 256 * 256 * a3 + 256 * 256 * 256 * a4);
                            if (tr == ud)
                            {
                                set_stat(p, 0, S_ORDER_UNIT_POINTER);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 1);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 2);
                                set_stat(p, 0, S_ORDER_UNIT_POINTER + 3);
                                give_order(p, 0, 0, ORDER_STOP);
                            }
                        }
                    }
                    p = (int*)(*((int*)((uintptr_t)p + S_NEXT_UNIT_POINTER)));
                }
            }
        }
    }

    if (*(byte*)LEVEL_OBJ == LVL_XORC5)
    {
        if (*((byte*)((uintptr_t)u + S_OWNER)) == P_BLUE)*(byte*)GB_HORSES = 1;
    }
}

PROC g_proc_00452939;
void grow_struct_count_tables(int* u)
{
    ((void (*)(int*))g_proc_00452939)(u);//original
}

void allow_table(byte p, int t, byte n, byte a)
{
    if (t == 0)t = ALLOWED_UNITS + (4 * p) + (n / 8);
    else if (t == 1)t = ALLOWED_UPGRADES + (4 * p) + (n / 8);
    else if (t == 2)t = ALLOWED_SPELLS + (4 * p) + (n / 8);
    else t = SPELLS_LEARNED + (4 * p) + (n / 8);
    byte b = *(byte*)t;
    if (a == 1)b |= (1 << (n % 8));
    else b &= (~(1 << (n % 8)));
    char buf[] = "\xff";
    buf[0] = b;
    PATCH_SET((char*)t, buf);
}

void draw_stats_fix(bool b)
{
    if (b)
    {
        char buf[] = "\xa0\x5b";
        PATCH_SET((char*)DEMON_STATS_DRAW, buf);//demon
        PATCH_SET((char*)CRITTER_STATS_DRAW, buf);//critter
    }
    else
    {
        char buf[] = "\xf0\x57";
        PATCH_SET((char*)DEMON_STATS_DRAW, buf);//demon
        PATCH_SET((char*)CRITTER_STATS_DRAW, buf);//critter
    }
}

void call_default_kill()//default kill all victory
{
    byte l = *(byte*)LOCAL_PLAYER;
    if (!slot_alive(l))lose(true);
    else
    {
        if (!check_opponents(l))win(true);
    }
}

//-------------files
const char FILES_PATH[] = ".\\BRUH\\";

void* pud_map1;
DWORD pud_map1_size;
void* pud_map2;
DWORD pud_map2_size;
void* pud_map3;
DWORD pud_map3_size;
void* pud_map4;
DWORD pud_map4_size;
void* pud_map5;
DWORD pud_map5_size;
void* pud_map6;
DWORD pud_map6_size;
void* pud_map7;
DWORD pud_map7_size;
void* pud_map8;
DWORD pud_map8_size;
void* pud_map9;
DWORD pud_map9_size;
void* pud_map10;
DWORD pud_map10_size;
void* pud_map11;
DWORD pud_map11_size;
void* pud_map12;
DWORD pud_map12_size;
void* pud_map13;
DWORD pud_map13_size;
void* pud_map14;
DWORD pud_map14_size;
void** pud_maps[] = { &pud_map1,&pud_map2,&pud_map3,&pud_map4,&pud_map5,&pud_map6,&pud_map7,&pud_map8,&pud_map9,&pud_map10,&pud_map11,&pud_map12,&pud_map13,&pud_map14 };
DWORD* pud_maps_size[] = { &pud_map1_size,&pud_map2_size,&pud_map3_size,&pud_map4_size,&pud_map5_size,&pud_map6_size,&pud_map7_size,&pud_map8_size,&pud_map9_size,&pud_map10_size,&pud_map11_size,&pud_map12_size,&pud_map13_size,&pud_map14_size };

void* pud_map15;
DWORD pud_map15_size;
void* pud_map16;
DWORD pud_map16_size;
void* pud_map17;
DWORD pud_map17_size;
void* pud_map18;
DWORD pud_map18_size;
void* pud_map19;
DWORD pud_map19_size;
void* pud_map20;
DWORD pud_map20_size;
void* pud_map21;
DWORD pud_map21_size;
void* pud_map22;
DWORD pud_map22_size;
void* pud_map23;
DWORD pud_map23_size;
void* pud_map24;
DWORD pud_map24_size;
void* pud_map25;
DWORD pud_map25_size;
void* pud_map26;
DWORD pud_map26_size;
void* pud_map27;
DWORD pud_map27_size;
void* pud_map28;
DWORD pud_map28_size;
void** pud_maps2[] = { &pud_map15,&pud_map16,&pud_map17,&pud_map18,&pud_map19,&pud_map20,&pud_map21,&pud_map22,&pud_map23,&pud_map24,&pud_map25,&pud_map26,&pud_map27,&pud_map28 };
DWORD* pud_maps2_size[] = { &pud_map15_size,&pud_map16_size,&pud_map17_size,&pud_map18_size,&pud_map19_size,&pud_map20_size,&pud_map21_size,&pud_map22_size,&pud_map23_size,&pud_map24_size,&pud_map25_size,&pud_map26_size,&pud_map27_size,&pud_map28_size };

void* pud_map29;
DWORD pud_map29_size;
void* pud_map30;
DWORD pud_map30_size;
void* pud_map31;
DWORD pud_map31_size;
void* pud_map32;
DWORD pud_map32_size;
void* pud_map33;
DWORD pud_map33_size;
void* pud_map34;
DWORD pud_map34_size;
void* pud_map35;
DWORD pud_map35_size;
void* pud_map36;
DWORD pud_map36_size;
void* pud_map37;
DWORD pud_map37_size;
void* pud_map38;
DWORD pud_map38_size;
void* pud_map39;
DWORD pud_map39_size;
void* pud_map40;
DWORD pud_map40_size;
void** pud_maps3[] = { &pud_map29,&pud_map30,&pud_map31,&pud_map32,&pud_map33,&pud_map34,&pud_map35,&pud_map36,&pud_map37,&pud_map38,&pud_map39,&pud_map40 };
DWORD* pud_maps3_size[] = { &pud_map29_size,&pud_map30_size,&pud_map31_size,&pud_map32_size,&pud_map33_size,&pud_map34_size,&pud_map35_size,&pud_map36_size,&pud_map37_size,&pud_map38_size,&pud_map39_size,&pud_map40_size };

void* pud_map41;
DWORD pud_map41_size;
void* pud_map42;
DWORD pud_map42_size;
void* pud_map43;
DWORD pud_map43_size;
void* pud_map44;
DWORD pud_map44_size;
void* pud_map45;
DWORD pud_map45_size;
void* pud_map46;
DWORD pud_map46_size;
void* pud_map47;
DWORD pud_map47_size;
void* pud_map48;
DWORD pud_map48_size;
void* pud_map49;
DWORD pud_map49_size;
void* pud_map50;
DWORD pud_map50_size;
void* pud_map51;
DWORD pud_map51_size;
void* pud_map52;
DWORD pud_map52_size;
void** pud_maps4[] = { &pud_map41,&pud_map42,&pud_map43,&pud_map44,&pud_map45,&pud_map46,&pud_map47,&pud_map48,&pud_map49,&pud_map50,&pud_map51,&pud_map52 };
DWORD* pud_maps4_size[] = { &pud_map41_size,&pud_map42_size,&pud_map43_size,&pud_map44_size,&pud_map45_size,&pud_map46_size,&pud_map47_size,&pud_map48_size,&pud_map49_size,&pud_map50_size,&pud_map51_size,&pud_map52_size };

void* bin_AI;
DWORD bin_AI_size;
void* bin_menu;
DWORD bin_menu_size;
void* bin_menu_copy;
void* bin_sngl;
DWORD bin_sngl_size;
void* bin_sngl_copy;
void* bin_newcmp;
DWORD bin_newcmp_size;
void* bin_newcmp_copy;

void* pcx_act;
void* pcx_act_pal;

PROC g_proc_004542FB;
int grp_draw_cross(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    //if level = orc1
    //if race = human
    //etc
    //new_grp = grp_runecircle;

    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_004542FB)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_004542FB)(a, u, grp, frame);//original
}

PROC g_proc_00454DB4;
int grp_draw_bullet(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    //44 - target

    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_00454DB4)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_00454DB4)(a, u, grp, frame);//original
}

PROC g_proc_00454BCA;
int grp_draw_unit(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------

    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_00454BCA)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_00454BCA)(a, u, grp, frame);//original
}

PROC g_proc_00455599;
int grp_draw_building(int a, int* u, void* grp, int frame)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    
    //-------------------------------------------------
    if (new_grp)
        return ((int (*)(int, int*, void*, int))g_proc_00455599)(a, u, new_grp, frame);
    else
        return ((int (*)(int, int*, void*, int))g_proc_00455599)(a, u, grp, frame);//original
}

PROC g_proc_0043AE54;
void grp_draw_building_placebox(void* grp, int frame, int a, int b)
{
    void* new_grp = NULL;
    int* peon = (int*)*(int*)LOCAL_UNITS_SELECTED;
    byte id = (*(int*)b) % 256;
    //-------------------------------------------------
    
    //-------------------------------------------------
    if (new_grp)
        ((void (*)(void*, int, int, int))g_proc_0043AE54)(new_grp, frame, a, b);
    else
        ((void (*)(void*, int, int, int))g_proc_0043AE54)(grp, frame, a, b);//original
}

int* portrait_unit;

PROC g_proc_0044538D;
void grp_portrait_init(int* a)
{
    ((void (*)(int*))g_proc_0044538D)(a);//original
    portrait_unit = (int*)*((int*)((uintptr_t)a + 0x26));
}

PROC g_proc_004453A7;//draw unit port
void grp_draw_portrait(void* grp, byte frame, int b, int c)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    int* u = portrait_unit;
    if (u != NULL)
    {

    }
    if (new_grp)
        return ((void (*)(void*, byte, int, int))g_proc_004453A7)(new_grp, frame, b, c);
    else
        return ((void (*)(void*, byte, int, int))g_proc_004453A7)(grp, frame, b, c);//original
}

PROC g_proc_004452B0;//draw buttons
void grp_draw_portrait_buttons(void* grp, byte frame, int b, int c)
{
    void* new_grp = NULL;
    //-------------------------------------------------
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (u != NULL)
    {

    }
    //-------------------------------------------------
    //new_grp = grp_port;

    if (new_grp)
        return ((void (*)(void*, byte, int, int))g_proc_004452B0)(new_grp, frame, b, c);
    else
        return ((void (*)(void*, byte, int, int))g_proc_004452B0)(grp, frame, b, c);//original
}

PROC g_proc_0044A65C;
int status_get_tbl(void* tbl, WORD str_id)
{
    int* u = (int*)*(int*)UNIT_STATUS_TBL;
    void* new_tbl = NULL;
    //-------------------------------------------------
    if (u != NULL)
    {

    }
    //-------------------------------------------------
    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0044A65C)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0044A65C)(tbl, str_id);//original

}

int* hover_unit;

PROC g_proc_0044AC83;
void unit_hover_get_id(int a, int* b)
{
    if (b != NULL)
    {
        byte id = *((byte*)((uintptr_t)b + 0x20));
        hover_unit = (int*)*(int*)(LOCAL_UNIT_SELECTED_PANEL + 4 * id);
    }
    else
        hover_unit = NULL;
    ((void (*)(int, int*))g_proc_0044AC83)(a, b);//original
}

PROC g_proc_0044AE27;
int unit_hover_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    int* u = hover_unit;
    if (u != NULL)
    {

    }
    //-------------------------------------------------
    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0044AE27)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0044AE27)(tbl, str_id);//original

}

PROC g_proc_0044AE56;
void button_description_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    /*
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (u != NULL)
    {

    }
    */
    //-------------------------------------------------
    //new_tbl = tbl_names;

    if (new_tbl)
        return ((void (*)(void*, int))g_proc_0044AE56)(new_tbl, str_id);
    else
        return ((void (*)(void*, int))g_proc_0044AE56)(tbl, str_id);//original

}

PROC g_proc_0044B23D;
void button_hotkey_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    /*
    int* u = (int*)*(int*)LOCAL_UNITS_SELECTED;
    if (u != NULL)
    {

    }
    */
    //-------------------------------------------------
    //new_tbl = tbl_names;

    if (new_tbl)
        return ((void (*)(void*, int))g_proc_0044B23D)(new_tbl, str_id);
    else
        return ((void (*)(void*, int))g_proc_0044B23D)(tbl, str_id);//original

}

void* tbl_credits;

char tbl_bruh[] = "bruh";

char tbl_bruhcraft1[] = "BruhCraft 1";
char tbl_bruhcraft2[] = "BruhCraft 2";
char tbl_bruhcraft3[] = "BruhCraft 3";
char tbl_bruhcraftA[] = "Bruhs and Bruhns";
char tbl_bruhcraftB[] = "Tides of Bruhhness";
char tbl_bruhcraftC[] = "Beyond Bruh Portal";
char tbl_bruhcraftD[] = "Reign of Bruh";
char tbl_bruhcraftE[] = "The Frozen Bruh";

char tbl_kill[] = "Kill all";
char tbl_mission[] = "                             ";

char tbl_obj1[] = " Kill all death knights\xA Destroy temple and runestone\xA Zuljin must survive";
//2
char tbl_obj3[] = " Kill all\xA Red Peon must survive";
char tbl_obj4[] = " Kill all dragons\xA Destroy all dragon lairs\xA Zuljin must survive";
char tbl_obj5[] = " Kill all\xA Alleria must survive";
char tbl_obj6[] = " Save all Heroes and bring them to circle";
char tbl_obj7[] = " Save Hadgar and bring to circle";
char tbl_obj8[] = " Kill all\xA Protect Runestone\xA All Heroes must survive";
//9
char tbl_obj10[] = " Kill all\xA Khadgar must survive";
char tbl_obj11[] = " Kill Deathwing and Teron\xA Only Tyralyon can kill Teron\xA Uter must survive\xA Tyralyon must survive";
char tbl_obj12[] = " Kill all critters";
char tbl_obj13[] = " Save all Groms and bring them to circles\xA Chogal must survive";
char tbl_obj14[] = " Kill all\xA Uter must survive";
//15
//16
char tbl_obj17[] = " Kill Green\xA Uter must survive";
char tbl_obj18[] = " Kill Violet\xA Protect Castle";
char tbl_obj19[] = " Kill Uter\xA Lotar must survive";
char tbl_obj20[] = " Kill all Kargaths\xA Grom must survive\xA Only Grom can kill Kargath";
char tbl_obj21[] = " Destroy Runestone";
char tbl_obj22[] = " Destroy Portal\xA Lotar and Uter must survive and get to circle";
//23
char tbl_obj24[] = " Destroy Khadgar\xA Only Dark Portal can destroy Khadgar";
//25
char tbl_obj26[] = " Kill Peon";
//27
char tbl_obj28[] = " Kill all\xA Lotar must survive\xA Uter must survive";

char tbl_obj29[] = " Kill all\xA All allied heroes must survive";
//30 = 29
char tbl_obj31[] = " Kill all\xA Capture portal\xA All allied heroes must survive";
//32 = 29
//33 = 29
//34 = 29
//35 = 29
char tbl_obj36[] = " Kill all\xA Mine 50000 gold\xA All allied heroes must survive";
char tbl_obj37[] = " Kill all\xA Only demon can kill Khadgar \xA Protect portal and runestone";
//38 = 29
//39 = 29
//40 = 29
//41
//42
char tbl_obj43[] = " Save Lotar and bring to circle\xA Lotar must survive";
//44
char tbl_obj45[] = " Kill all execpt blue\xA do NOT kill any blue units";
//46
char tbl_obj47[] = " Reach food limit of 250/200 units (or more)";
char tbl_obj48[] = " Kill all\xA All white peasants must survive";
char tbl_obj49[] = " Destroy blue castle";
char tbl_obj50[] = " Kill all\xA Khadgar must survive";
char tbl_obj51[] = " Kill all\xA Uter must survive\xA Khadgar must survive";
char tbl_obj52[] = " Final battle of bruh for the most bruhhess one!!!";

int ret_tbl_obj()
{
    byte lvl = *(byte*)LEVEL_OBJ;
    if (lvl == LVL_HUMAN1)return(int)tbl_obj1;
    //2
    else if (lvl == LVL_HUMAN3)return(int)tbl_obj3;
    else if (lvl == LVL_HUMAN4)return(int)tbl_obj4;
    else if (lvl == LVL_HUMAN5)return(int)tbl_obj5;
    else if (lvl == LVL_HUMAN6)return(int)tbl_obj6;
    else if (lvl == LVL_HUMAN7)return(int)tbl_obj7;
    else if (lvl == LVL_HUMAN8)return(int)tbl_obj8;
    //9
    else if (lvl == LVL_HUMAN10)return(int)tbl_obj10;
    else if (lvl == LVL_HUMAN11)return(int)tbl_obj11;
    else if (lvl == LVL_HUMAN12)return(int)tbl_obj12;
    else if (lvl == LVL_HUMAN13)return(int)tbl_obj13;
    else if (lvl == LVL_HUMAN14)return(int)tbl_obj14;
    //15
    //16
    else if (lvl == LVL_ORC3)return(int)tbl_obj17;
    else if (lvl == LVL_ORC4)return(int)tbl_obj18;
    else if (lvl == LVL_ORC5)return(int)tbl_obj19;
    else if (lvl == LVL_ORC6)return(int)tbl_obj20;
    else if (lvl == LVL_ORC7)return(int)tbl_obj21;
    else if (lvl == LVL_ORC8)return(int)tbl_obj22;
    //23
    else if (lvl == LVL_ORC10)return(int)tbl_obj24;
    //25
    else if (lvl == LVL_ORC12)return(int)tbl_obj26;
    //27
    else if (lvl == LVL_ORC14)return(int)tbl_obj28;
    else if (lvl == LVL_XHUMAN1)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN2)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN3)return(int)tbl_obj31;
    else if (lvl == LVL_XHUMAN4)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN5)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN6)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN7)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN8)return(int)tbl_obj36;
    else if (lvl == LVL_XHUMAN9)return(int)tbl_obj37;
    else if (lvl == LVL_XHUMAN10)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN11)return(int)tbl_obj29;
    else if (lvl == LVL_XHUMAN12)return(int)tbl_obj29;
    //41
    //42
    else if (lvl == LVL_XORC3)return(int)tbl_obj43;
    //44
    else if (lvl == LVL_XORC5)return(int)tbl_obj45;
    //46
    else if (lvl == LVL_XORC7)return(int)tbl_obj47;
    else if (lvl == LVL_XORC8)return(int)tbl_obj48;
    else if (lvl == LVL_XORC9)return(int)tbl_obj49;
    else if (lvl == LVL_XORC10)return(int)tbl_obj50;
    else if (lvl == LVL_XORC11)return(int)tbl_obj51;
    else if (lvl == LVL_XORC12)return(int)tbl_obj52;

    else return (int)tbl_kill;
}

PROC g_proc_004354C8;
int objct_get_tbl_custom(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_kill;
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004354C8)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004354C8)(tbl, str_id);//original
}

PROC g_proc_004354FA;
int objct_get_tbl_campanign(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    return ret_tbl_obj();
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004354FA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004354FA)(tbl, str_id);//original
}

PROC g_proc_004300A5;
int objct_get_tbl_briefing_task(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    return ret_tbl_obj();
    //-------------------------------------------------
    //new_tbl = tbl_obj;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004300A5)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004300A5)(tbl, str_id);//original
}

PROC g_proc_004300CA;
int objct_get_tbl_briefing_title(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl <= LVL_ORC14)
    {
        if ((lvl % 2) == 0)sprintf(tbl_mission, "mission %d", (lvl / 2) + 1);
        else sprintf(tbl_mission, "mission %d", (lvl / 2) + 1 + 14);
    }
    else
    {
        if ((lvl % 2) == 0)sprintf(tbl_mission, "mission %d", ((lvl - LVL_XHUMAN1) / 2) + 1 + 28);
        else sprintf(tbl_mission, "mission %d", ((lvl - LVL_XHUMAN1) / 2) + 1 + 40);
    }
    return (int)tbl_mission;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004300CA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004300CA)(tbl, str_id);//original
}

PROC g_proc_004301CA;
int objct_get_tbl_briefing_text(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    return (int)tbl_bruh;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004301CA)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004301CA)(tbl, str_id);//original
}

void set_speech(char* speech, char* adr)
{
    patch_setdword((DWORD*)(speech + 4), (DWORD)adr);
    patch_setdword((DWORD*)(speech + 12), 0);
}

DWORD remember_music = 101;
DWORD remember_sound = 101;

PROC g_proc_00430113;
int objct_get_briefing_speech(char* speech)
{
    remember_music = *(DWORD*)VOLUME_MUSIC;
    remember_sound = *(DWORD*)VOLUME_SOUND;
    if (remember_music != 0)
        *(DWORD*)VOLUME_MUSIC = 20;
    *(DWORD*)VOLUME_SOUND = 100;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume

    DWORD remember1 = *(DWORD*)(speech + 4);
    DWORD remember2 = *(DWORD*)(speech + 12);
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    
    //-------------------------------------------------

    //int original = ((int (*)(char*))g_proc_00430113)(speech);//original
    patch_setdword((DWORD*)(speech + 4), remember1);
    patch_setdword((DWORD*)(speech + 12), remember2);
    //return original;
    return 0;
}

bool finale_dlg = false;

PROC g_proc_0041F0F5;
int finale_get_tbl(void* tbl, WORD str_id)
{
    finale_dlg = false;
    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_bruh;
    //-------------------------------------------------
    //new_tbl = tbl_end;
    //str_id = 1;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041F0F5)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041F0F5)(tbl, str_id);//original
}

PROC g_proc_0041F1E8;
int finale_credits_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    //return (int)tbl_bruh;
    //-------------------------------------------------
    new_tbl = tbl_credits;
    str_id = 1;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041F1E8)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041F1E8)(tbl, str_id);//original
}

PROC g_proc_0041F027;
int finale_get_speech(char* speech)
{
    remember_music = *(DWORD*)VOLUME_MUSIC;
    remember_sound = *(DWORD*)VOLUME_SOUND;
    //if (remember_music != 0)*(DWORD*)VOLUME_MUSIC = 0;
    *(DWORD*)VOLUME_SOUND = 0;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume

    DWORD remember1 = *(DWORD*)(speech + 4);
    DWORD remember2 = *(DWORD*)(speech + 12);
    //-------------------------------------------------
    //set_speech(speech, story_end);
    //-------------------------------------------------
    int original = ((int (*)(char*))g_proc_0041F027)(speech);//original
    patch_setdword((DWORD*)(speech + 4), remember1);
    patch_setdword((DWORD*)(speech + 12), remember2);
    return original;
    //return 0;
}

int cred_num = 0;

PROC g_proc_00417E33;
int credits_small_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    //return (int)tbl_bruh;
    //-------------------------------------------------
    new_tbl = tbl_credits;
    str_id = 1;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00417E33)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00417E33)(tbl, str_id);//original
}

PROC g_proc_00417E4A;
int credits_big_get_tbl(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_bruh;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00417E4A)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00417E4A)(tbl, str_id);//original
}

PROC g_proc_0042968A;
int act_get_tbl_small(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl <= LVL_ORC14)
    {
        if ((lvl % 2) == 0)
        {
            if (lvl <= LVL_HUMAN8)return (int)tbl_bruhcraft1;
            else return (int)tbl_bruhcraft2;
        }
        else return (int)tbl_bruhcraft2;
    }
    else return (int)tbl_bruhcraft3;
    return (int)tbl_bruh;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0042968A)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0042968A)(tbl, str_id);//original
}

PROC g_proc_004296A9;
int act_get_tbl_big(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    byte lvl = *(byte*)LEVEL_OBJ;
    //-------------------------------------------------
    if (lvl <= LVL_ORC14)
    {
        if ((lvl % 2) == 0)
        {
            if (lvl <= LVL_HUMAN8)return (int)tbl_bruhcraftA;
            else return (int)tbl_bruhcraftB;
        }
        else return (int)tbl_bruhcraftC;
    }
    else
    {
        if ((lvl % 2) == 0)return (int)tbl_bruhcraftD;
        else return (int)tbl_bruhcraftE;
    }
    return (int)tbl_bruh;
    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_004296A9)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_004296A9)(tbl, str_id);//original
}

PROC g_proc_0041C51C;
int netstat_get_tbl_nation(void* tbl, WORD str_id)
{
    void* new_tbl = NULL;
    //-------------------------------------------------

    //-------------------------------------------------

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_0041C51C)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_0041C51C)(tbl, str_id);//original
}

PROC g_proc_00431229;
int rank_get_tbl(void* tbl, WORD str_id)
{
    if (*(byte*)LEVEL_OBJ == LVL_XORC12)*(byte*)PLAYER_RACE = 1;//orc = 1

    if (*(byte*)LEVEL_OBJ == LVL_HUMAN14)
    {
        *(byte*)LEVEL_OBJ = 254 + LVL_ORC1;
        *(WORD*)LEVEL_ID = 0x52C7;
    }
    else if (*(byte*)LEVEL_OBJ == LVL_ORC14)
    {
        *(byte*)LEVEL_OBJ = LVL_XHUMAN1 - 2;
        *(WORD*)LEVEL_ID = 0x53C6 - 2;
    }
    else if (*(byte*)LEVEL_OBJ == LVL_XHUMAN12)
    {
        *(byte*)LEVEL_OBJ = LVL_XORC1 - 2;
        *(WORD*)LEVEL_ID = 0x53C7 - 2;
    }

    void* new_tbl = NULL;
    //-------------------------------------------------
    return (int)tbl_bruh;
    //-------------------------------------------------
    //new_tbl = tbl_rank;

    if (new_tbl)
        return ((int (*)(void*, int))g_proc_00431229)(new_tbl, str_id);
    else
        return ((int (*)(void*, int))g_proc_00431229)(tbl, str_id);//original
}

bool pcx_save_raw = false;

void pal_load(byte* palette_adr, void* pal)
{
    if (palette_adr != NULL)
    {
        if (pal != NULL)
        {
            DWORD i = 0;
            while (i < (256 * 4))
            {
                *(byte*)(palette_adr + i) = *(byte*)((DWORD)pal + i);
                i++;
            }
        }
    }
}

PROC g_proc_004372EE;
void pcx_load_menu(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_004372EE)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL; 
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\titlemenu_bne.pcx", -1) == CSTR_EQUAL)
    {
        cred_num = 0;
        //new_pcx_pixels = pcx_menu;
        //pal_load(palette_adr, pcx_menu_pal);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\title_rel_bne.pcx", -1) == CSTR_EQUAL)
    {
        if (pcx_save_raw)
        {
            byte* adr = (byte*)*(DWORD*)((DWORD)pcx_info + 4);
            byte data_pcx[640 * 480];
            byte data_pcx_pal[256 * 4];
            DWORD i = 0;
            while (i < (640 * 480))
            {
                data_pcx[i] = *(char*)(adr + i);
                i++;
            }
            FILE* fp;
            if ((fp = fopen("title_rel.raw", "a+b")) != NULL)
            {
                fwrite(data_pcx, sizeof(byte), 640 * 480, fp);
                fclose(fp);
            }
            i = 0;
            while (i < (256 * 4))
            {
                data_pcx_pal[i] = *(byte*)(palette_adr + i);
                i++;
            }
            if ((fp = fopen("title_rel.pal", "a+b")) != NULL)
            {
                fwrite(data_pcx_pal, sizeof(byte), 256 * 4, fp);
                fclose(fp);
            }
        }
        else
        {
            //new_pcx_pixels = pcx_splash;
            //pal_load(palette_adr, pcx_splash_pal);
        }
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00430058;
void pcx_load_briefing(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00430058)(name, pcx_info, palette_adr);//original
    byte lvl = *(byte*)LEVEL_OBJ;
    void* new_pcx_pixels = NULL;

    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00429625;//load palette
PROC g_proc_00429654;//load image
void pcx_load_act(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00429625)(name, pcx_info, palette_adr);//original
    byte lvl = *(byte*)LEVEL_OBJ;
    void* new_pcx_pixels = NULL;

    new_pcx_pixels = pcx_act;
    pal_load(palette_adr, pcx_act_pal);

    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0041F004;
void pcx_load_final(char* name, void* pcx_info, byte* palette_adr)
{
    finale_dlg = true;
    ((void (*)(char*, void*, byte*))g_proc_0041F004)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    //new_pcx_pixels = pcx_end;
    //pal_load(palette_adr, pcx_end_pal);
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00417DDB;
void pcx_load_credits(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_00417DDB)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\credits.pcx", -1) == CSTR_EQUAL)
    {
        if (cred_num == 0)
        {
            cred_num = 1; 
            //new_pcx_pixels = pcx_credit1;
            //pal_load(palette_adr, pcx_credit1_pal);
        }
        if (cred_num == 2)
        {
            cred_num = 3;
            //new_pcx_pixels = pcx_credit3;
            //pal_load(palette_adr, pcx_credit3_pal);
        }
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\2xcredit.pcx", -1) == CSTR_EQUAL)
    {
        cred_num = 2;
        //new_pcx_pixels = pcx_credit2;
        //pal_load(palette_adr, pcx_credit2_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_0043169E;
void pcx_load_statistic(char* name, void* pcx_info, byte* palette_adr)
{
    ((void (*)(char*, void*, byte*))g_proc_0043169E)(name, pcx_info, palette_adr);//original
    void* new_pcx_pixels = NULL;
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hwinglu.pcx", -1) == CSTR_EQUAL)
    {
        //new_pcx_pixels = pcx_win;
        //pal_load(palette_adr, pcx_win_pal);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "art\\hlosglu.pcx", -1) == CSTR_EQUAL)
    {
        //new_pcx_pixels = pcx_loss;
        //pal_load(palette_adr, pcx_loss_pal);
    }
    if (new_pcx_pixels)patch_setdword((DWORD*)((DWORD)pcx_info + 4), (DWORD)new_pcx_pixels);
}

PROC g_proc_00462D4D;
void* storm_file_load(char* name, int a1, int a2, int a3, int a4, int a5, int a6)
{
    void* original = ((void* (*)(void*, int, int, int, int, int, int))g_proc_00462D4D)(name, a1, a2, a3, a4, a5, a6);//original
    void* new_file = NULL;
    //-------------------------------------------------
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\mainmenu.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_menu_copy, bin_menu, bin_menu_size);
        new_file = bin_menu_copy;
        patch_setdword((DWORD*)a2, bin_menu_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\snglplay.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_sngl_copy, bin_sngl, bin_sngl_size);
        new_file = bin_sngl_copy;
        patch_setdword((DWORD*)a2, bin_sngl_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\newcmpgn.bin", -1) == CSTR_EQUAL)
    {
        memcpy(bin_newcmp_copy, bin_newcmp, bin_newcmp_size);
        new_file = bin_newcmp_copy;
        patch_setdword((DWORD*)a2, bin_newcmp_size);
    }
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, name, -1, "rez\\ai.bin", -1) == CSTR_EQUAL)
    {
        new_file = bin_AI;
    }
    //-------------------------------------------------
    if (new_file)
        return new_file;
    else
        return original;
}

void set_rgb(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, r + (g << 8) + (b << 16));
}

void set_rgb4(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (4 * r) + ((4 * g) << 8) + ((4 * b) << 16));
}

void set_rgb8(DWORD* adr, byte r, byte g, byte b)
{
    patch_setdword(adr, (8 * r) + ((8 * g) << 8) + ((8 * b) << 16));
}

void set_palette(void* pal)
{
    for (int i = 0; i < 256; i++)
    {
        byte r = *(byte*)((int)pal + 3 * i);
        byte g = *(byte*)((int)pal + 3 * i + 1);
        byte b = *(byte*)((int)pal + 3 * i + 2);
        set_rgb4((DWORD*)(SCREEN_INITIAL_PALETTE + i * 4), r, g, b);
    }
}

void set_yellow_palette()
{
    //change human blue buttons to yellowish
    int k = 0xB0;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 4, 4, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 6, 6, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 8, 8, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 9, 9, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 11, 11, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 14, 14, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 15, 15, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 16, 16, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 18, 18, 0); k++;
    set_rgb8((DWORD*)(SCREEN_INITIAL_PALETTE + k * 4), 19, 19, 0);
}

PROC g_proc_0041F9FD;
int tilesets_load(int a)
{
    int original = ((int (*)(int))g_proc_0041F9FD)(a);
    byte lvl = *(byte*)LEVEL_OBJ;

    set_yellow_palette();
    return original;
}

PROC g_proc_0041F97D;
int map_file_load(int a, int b, void** map, DWORD* size)
{
    byte lvl = *(byte*)LEVEL_OBJ;
    byte d = *(byte*)GB_DEMO;//demo
    bool f = false;
    if (d == 1)
    {
        //*map = pud_map_atr;
        //*size = pud_map_atr_size;
        //f = true;
    }
    else
    {
        if ((lvl >= LVL_HUMAN1) && (lvl <= LVL_ORC14))
        {
            if ((lvl % 2) == 0)
            {
                *map = *pud_maps[lvl / 2];
                *size = *pud_maps_size[lvl / 2];
            }
            else
            {
                *map = *pud_maps2[lvl / 2];
                *size = *pud_maps2_size[lvl / 2];
            }
            f = true;
        }
        if ((lvl >= LVL_XHUMAN1) && (lvl <= LVL_XORC12))
        {
            if ((lvl % 2) == 0)
            {
                *map = *pud_maps3[(lvl - LVL_XHUMAN1) / 2];
                *size = *pud_maps3_size[(lvl - LVL_XHUMAN1) / 2];
            }
            else
            {
                *map = *pud_maps4[(lvl - LVL_XHUMAN1) / 2];
                *size = *pud_maps4_size[(lvl - LVL_XHUMAN1) / 2];
            }
            f = true;
        }
    }
    if (f)return 1;
    else return ((int (*)(int, int, void*, DWORD*))g_proc_0041F97D)(a, b, map, size);//original
}

void* file_load(const char name[])
{
    void* file = NULL;
    FILE* fp;
    char path[MAX_PATH] = { 0 };
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL)//file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp);//read
        fclose(fp);
    }
    return file;
}

void file_load_size(const char name[], void** m, DWORD* s)
{
    void* file = NULL;
    FILE* fp;
    char path[MAX_PATH] = { 0 };
    _snprintf(path, sizeof(path), "%s%s", FILES_PATH, name);
    if ((fp = fopen(path, "rb")) != NULL)//file opened
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        DWORD size = ftell(fp); // get current file pointer
        *s = size;
        fseek(fp, 0, SEEK_SET); // seek back to beginning of file
        file = malloc(size);
        fread(file, sizeof(unsigned char), size, fp);//read
        fclose(fp);
    }
    *m = file;
}

char video_mod_shab[] = "%s";
char video1[] = "\xA";

PROC g_proc_0043B16F;
void smk_play_sprintf_name(int dest, char* shab, char* name)
{
    if (!lstrcmpi(name, "orcx_m.smk"))
    {
        shab = video_mod_shab;
        name = video1;
    }
    ((void (*)(int, char*, char*))g_proc_0043B16F)(dest, shab, name);//original
}

PROC g_proc_0043B362;
void smk_play_sprintf_blizzard(int dest, char* shab, char* name)
{
    //((void (*)(int, char*, char*))g_proc_0043B362)(dest, video_mod_shab, video1);//original
    ((void (*)(int, char*, char*))g_proc_0043B362)(dest, shab, name);//original
}

DWORD loaded_instal = 0;
int need_instal = 0;

void reload_install_exe()
{
    //if (loaded_instal == 0)
    //{
    //    *(DWORD*)INSTALL_EXE_POINTER = 0;//remove existing install
    //    char buf[] = "\x0\x0\x0\x0";
    //    patch_setdword((DWORD*)buf, (DWORD)instal);
    //    PATCH_SET((char*)INSTALL_EXE_NAME1, buf);
    //    PATCH_SET((char*)INSTALL_EXE_NAME2, buf);//change names
    //    PATCH_SET((char*)INSTALL_EXE_NAME3, buf);
    //    ((int (*)(int, int))F_RELOAD_INSTALL_EXE)(1, 0);//load install.exe
    //    loaded_instal = *(DWORD*)INSTALL_EXE_POINTER;
    //}
    //else
    //{
    //    *(DWORD*)INSTALL_EXE_POINTER = loaded_instal;
    //}
}

void music_play_sprintf_name(int dest, char* shab, char* name)
{
    DWORD orig = *(DWORD*)F_MUSIC_SPRINTF;//original music sprintf call
    ((void (*)(int, char*, char*))orig)(dest, shab, name);//original
}

PROC g_proc_00440F4A;
PROC g_proc_00440F5F;
int music_play_get_install()
{
    //DWORD orig_instal = *(DWORD*)INSTALL_EXE_POINTER;//remember existing install
    //if (need_instal == 1)
    //{
    //    reload_install_exe();
    //    need_instal--;
    //}
    //if (need_instal == 2)need_instal--;
    int original = ((int (*)())g_proc_00440F4A)();//original
    //*(DWORD*)INSTALL_EXE_POINTER = orig_instal;//restore existing install
    return original;
}

void files_init()
{
    tbl_credits = file_load("credits.tbl");

    file_load_size("map1.pud", &pud_map1, &pud_map1_size);
    file_load_size("map2.pud", &pud_map2, &pud_map2_size);
    file_load_size("map3.pud", &pud_map3, &pud_map3_size);
    file_load_size("map4.pud", &pud_map4, &pud_map4_size);
    file_load_size("map5.pud", &pud_map5, &pud_map5_size);
    file_load_size("map6.pud", &pud_map6, &pud_map6_size);
    file_load_size("map7.pud", &pud_map7, &pud_map7_size);
    file_load_size("map8.pud", &pud_map8, &pud_map8_size);
    file_load_size("map9.pud", &pud_map9, &pud_map9_size);
    file_load_size("map10.pud", &pud_map10, &pud_map10_size);
    file_load_size("map11.pud", &pud_map11, &pud_map11_size);
    file_load_size("map12.pud", &pud_map12, &pud_map12_size);
    file_load_size("map13.pud", &pud_map13, &pud_map13_size);
    file_load_size("map14.pud", &pud_map14, &pud_map14_size);
    file_load_size("map15.pud", &pud_map15, &pud_map15_size);

    file_load_size("map15.pud", &pud_map15, &pud_map15_size);
    file_load_size("map16.pud", &pud_map16, &pud_map16_size);
    file_load_size("map17.pud", &pud_map17, &pud_map17_size);
    file_load_size("map18.pud", &pud_map18, &pud_map18_size);
    file_load_size("map19.pud", &pud_map19, &pud_map19_size);
    file_load_size("map20.pud", &pud_map20, &pud_map20_size);
    file_load_size("map21.pud", &pud_map21, &pud_map21_size);
    file_load_size("map22.pud", &pud_map22, &pud_map22_size);
    file_load_size("map23.pud", &pud_map23, &pud_map23_size);
    file_load_size("map24.pud", &pud_map24, &pud_map24_size);
    file_load_size("map25.pud", &pud_map25, &pud_map25_size);
    file_load_size("map26.pud", &pud_map26, &pud_map26_size);
    file_load_size("map27.pud", &pud_map27, &pud_map27_size);
    file_load_size("map28.pud", &pud_map28, &pud_map28_size);

    file_load_size("map29.pud", &pud_map29, &pud_map29_size);
    file_load_size("map30.pud", &pud_map30, &pud_map30_size);
    file_load_size("map31.pud", &pud_map31, &pud_map31_size);
    file_load_size("map32.pud", &pud_map32, &pud_map32_size);
    file_load_size("map33.pud", &pud_map33, &pud_map33_size);
    file_load_size("map34.pud", &pud_map34, &pud_map34_size);
    file_load_size("map35.pud", &pud_map35, &pud_map35_size);
    file_load_size("map36.pud", &pud_map36, &pud_map36_size);
    file_load_size("map37.pud", &pud_map37, &pud_map37_size);
    file_load_size("map38.pud", &pud_map38, &pud_map38_size);
    file_load_size("map39.pud", &pud_map39, &pud_map39_size);
    file_load_size("map40.pud", &pud_map40, &pud_map40_size);

    file_load_size("map41.pud", &pud_map41, &pud_map41_size);
    file_load_size("map42.pud", &pud_map42, &pud_map42_size);
    file_load_size("map43.pud", &pud_map43, &pud_map43_size);
    file_load_size("map44.pud", &pud_map44, &pud_map44_size);
    file_load_size("map45.pud", &pud_map45, &pud_map45_size);
    file_load_size("map46.pud", &pud_map46, &pud_map46_size);
    file_load_size("map47.pud", &pud_map47, &pud_map47_size);
    file_load_size("map48.pud", &pud_map48, &pud_map48_size);
    file_load_size("map49.pud", &pud_map49, &pud_map49_size);
    file_load_size("map50.pud", &pud_map50, &pud_map50_size);
    file_load_size("map51.pud", &pud_map51, &pud_map51_size);
    file_load_size("map52.pud", &pud_map52, &pud_map52_size);

    file_load_size("aied.bin", &bin_AI, &bin_AI_size);
    file_load_size("menu.bin", &bin_menu, &bin_menu_size);
    file_load_size("menu.bin", &bin_menu_copy, &bin_menu_size);
    file_load_size("sngl.bin", &bin_sngl, &bin_sngl_size);
    file_load_size("sngl.bin", &bin_sngl_copy, &bin_sngl_size);
    file_load_size("newcmp.bin", &bin_newcmp, &bin_newcmp_size);
    file_load_size("newcmp.bin", &bin_newcmp_copy, &bin_newcmp_size);

    pcx_act = file_load("act.raw");
    pcx_act_pal = file_load("act.pal");
}

PROC g_proc_0042A443;
void act_init()
{
    WORD m = *(WORD*)LEVEL_ID;
    //if (*(byte*)LEVEL_OBJ == LVL_HUMAN1)*(WORD*)LEVEL_ID = 0x52D0;
    //else *(WORD*)LEVEL_ID = 0x52C8;//mission file number
    *(WORD*)LEVEL_ID = 0x52C8;//mission file number
    *(WORD*)PREVIOUS_ACT = 0;//prev act
    ((void (*)())g_proc_0042A443)();//original
    *(WORD*)LEVEL_ID = m;//mission file number restore

    if (*(byte*)LEVEL_OBJ == LVL_ORC1)
    {
        //ShellExecute(NULL, "open", ".\\BRUH\\New World.txt", NULL, NULL, SW_SHOWDEFAULT);
    }
    if (*(byte*)LEVEL_OBJ == LVL_XORC1)
    {
        //ShellExecute(NULL, "open", ".\\BRUH\\Khadgar's Journal.pdf", NULL, NULL, SW_SHOWDEFAULT);
    }
}

PROC g_proc_00422D76;
void sound_play_unit_speech(WORD sid, int a, int* u, int b)
{
    bool f = true;
    if (u != NULL)
    {

    }
    if (f)((void (*)(WORD, int, int*, int))g_proc_00422D76)(sid, a, u, b);//original
}

PROC g_proc_00422D5F;
void sound_play_unit_speech_soft(WORD sid, int a, int* u, int b)
{
    bool f = true;
    if (u != NULL)
    {

    }
    if (f)((void (*)(WORD, int, int*, int))g_proc_00422D5F)(sid, a, u, b);//original
}

PROC g_proc_00402811;
void button_play_sound()
{
    def_sound = (void*)*(int*)0x004BBA00;//save default
    if (!game_started)patch_setdword((DWORD*)0x004BBA00, (DWORD)bruh_sound);
    ((void (*)())g_proc_00402811)();//original
    patch_setdword((DWORD*)0x004BBA00, (DWORD)def_sound);//restore default

}
//-------------files

PROC g_proc_0044F37D;
void main_menu_init(int a)
{
    if (remember_music != 101)
        *(DWORD*)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD*)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume
    remember_music = 101;
    remember_sound = 101;

    *(byte*)PLAYER_RACE = 0;//human cursor

    int sid = 37;//37 button.wav
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);//save default
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)bruh_name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)bruh_sound);
    ((void (*)(int, int, int, int))0x004406B0)(sid, 0xFFFFFF35, 0, 255);
    bruh_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid), (DWORD)def_sound);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name);//restore default
    game_started = false;

    ((void (*)(int))g_proc_0044F37D)(a);
}

PROC g_proc_00418937;
void dispatch_die_unitdraw_update_1_man(int* u)
{
    ((void (*)(int*))g_proc_00418937)(u);//original
}

PROC g_proc_00451590;
void unit_kill_peon_change(int* u)
{
    ((void (*)(int*))g_proc_00451590)(u);//original
}

void def_stat(byte u, WORD hp, byte str, byte prc, byte arm, byte rng, byte gold, byte lum, byte oil, byte time)
{
    //change some unit stats (changes for ALL units of this type)

    /*
    to change vision and multiselectable you can use this construction
    char buf[] = "\x0\x0\x0\x0";//fix vision
    patch_setdword((DWORD*)buf, (DWORD)F_VISION6);
    PATCH_SET((char*)(UNIT_VISION_FUNCTIONS_TABLE + 4 * U_DEMON), buf);
    char buf2[] = "\x1";
    PATCH_SET((char*)(UNIT_MULTISELECTABLE + U_DEMON), buf2);
    */

    char buf2[] = "\x0\x0";
    buf2[0] = hp % 256;
    buf2[1] = hp / 256;
    PATCH_SET((char*)(UNIT_HP_TABLE + 2 * u), buf2);
    char buf[] = "\x0";
    buf[0] = str;
    PATCH_SET((char*)(UNIT_STRENGTH_TABLE + u), buf);
    buf[0] = prc;
    PATCH_SET((char*)(UNIT_PIERCE_TABLE + u), buf);
    buf[0] = arm;
    PATCH_SET((char*)(UNIT_ARMOR_TABLE + u), buf);
    if (rng != 0)
    {
        buf[0] = rng;
        PATCH_SET((char*)(UNIT_RANGE_TABLE + u), buf);
    }
    buf[0] = gold;
    PATCH_SET((char*)(UNIT_GOLD_TABLE + u), buf);
    buf[0] = lum;
    PATCH_SET((char*)(UNIT_LUMBER_TABLE + u), buf);
    buf[0] = oil;
    PATCH_SET((char*)(UNIT_OIL_TABLE + u), buf);
    buf[0] = time;
    PATCH_SET((char*)(UNIT_TIME_TABLE + u), buf);
}

void def_upgr(byte u, WORD gold, WORD lum, WORD oil)
{
    char buf2[] = "\x0\x0";
    buf2[0] = gold % 256;
    buf2[1] = gold / 256;
    PATCH_SET((char*)(UPGR_GOLD_TABLE + 2 * u), buf2);
    buf2[0] = lum % 256;
    buf2[1] = lum / 256;
    PATCH_SET((char*)(UPGR_LUMBER_TABLE + 2 * u), buf2);
    buf2[0] = oil % 256;
    buf2[1] = oil / 256;
    PATCH_SET((char*)(UPGR_OIL_TABLE + 2 * u), buf2);
}

PROC g_proc_0041F915;
int map_load(void* map, DWORD size)
{
    int original = ((int (*)(void*, DWORD))g_proc_0041F915)(map, size);

    byte lvl = *(byte*)LEVEL_OBJ;

    return original;
}

PROC g_proc_0042BB04;
int* map_create_unit(int x, int y, byte id, byte o)
{
    int* u = NULL;
    u = ((int* (*)(int, int, byte, byte))g_proc_0042BB04)(x, y, id, o);
    if (u != NULL)
    {
        if (ai_fixed)
        {
            if ((id == U_PEASANT) || (id == U_PEON))
            {
                set_stat(u, 255, S_NEXT_FIRE);
                set_stat(u, 2, S_NEXT_FIRE + 1);
            }
        }
    }
    return u;
}

PROC g_proc_00424745;//entering
PROC g_proc_004529C0;//grow struct
int goods_into_inventory(int* p)
{
    /*int tr = (*(int*)((uintptr_t)p + S_ORDER_UNIT_POINTER));
    if (tr != 0)
    {
        bool f = false;
        int* trg = (int*)tr;
        byte o = *((byte*)((uintptr_t)p + S_OWNER));
        byte id = *((byte*)((uintptr_t)p + S_ID));
        byte tid = *((byte*)((uintptr_t)trg + S_ID));
        byte pf = *((byte*)((uintptr_t)p + S_PEON_FLAGS));
        int pflag = *(int*)(UNIT_GLOBAL_FLAGS + id * 4);
        int tflag = *(int*)(UNIT_GLOBAL_FLAGS + tid * 4);
        int res = 100;
        if (pf & PEON_LOADED)
        {
            if (((pflag & IS_SHIP) != 0) && ((tflag & IS_OILRIG) == 0))
            {
                int r = get_val(REFINERY, o);
                if (r != 0)res = 125;
                else res = 100;
                change_res(o, 2, 1, res);
                add_total_res(o, 2, 1, res);
                f = true;
            }
            else
            {
                if (((tflag & IS_TOWNHALL) != 0) || ((tflag & IS_LUMBER) != 0))
                {
                    if (((tflag & IS_TOWNHALL) != 0))
                    {
                        pf |= PEON_IN_CASTLE;
                        set_stat(p, pf, S_PEON_FLAGS);
                    }
                    if (((pf & PEON_HARVEST_GOLD) != 0) && ((tflag & IS_TOWNHALL) != 0))
                    {
                        int r2 = get_val(TH2, o);
                        int r3 = get_val(TH3, o);
                        if (r3 != 0)res = 120;
                        else
                        {
                            if (r2 != 0)res = 110;
                            else res = 100;
                        }
                        if (o == *(byte*)LOCAL_PLAYER)
                        {
                            if (get_upgrade(CATA_DMG, o) != 0)
                                res += 10;
                            if (get_val(SMITH, o) != 0)
                                res += 10;
                            if (*(byte*)(GB_RANGER + o) == 1)
                                res += 10;
                        }
                        pf &= ~PEON_HARVEST_GOLD;
                        change_res(o, 0, 1, res);
                        add_total_res(o, 0, 1, res);
                        f = true;
                    }
                    else
                    {
                        if (((pf & PEON_HARVEST_LUMBER) != 0))
                        {
                            int r = get_val(LUMBERMILL, o);
                            if (r != 0)res = 125;
                            else res = 100;
                            if (o == *(byte*)LOCAL_PLAYER)
                            {
                                if (*(byte*)(GB_LONGBOW + o) == 1)
                                    res += 25;
                            }
                            pf &= ~PEON_HARVEST_LUMBER;
                            change_res(o, 1, 1, res);
                            add_total_res(o, 1, 1, res);
                            f = true;
                        }
                    }
                }
            }
        }
            if (f)
            {
                pf &= ~PEON_LOADED;
                set_stat(p, pf, S_PEON_FLAGS);
                ((void (*)(int*))F_GROUP_SET)(p);
                return 1;
            }
    }
    return 0;*/
    return ((int(*)(int*))g_proc_00424745)(p);//original
}

PROC g_proc_0042479E;
void peon_into_goldmine(int* u)
{
    ((void (*)(int*))g_proc_0042479E)(u);//original
    byte lvl = *(byte*)LEVEL_OBJ;
    byte o = *((byte*)((uintptr_t)u + S_OWNER));
    if (*(byte*)(CONTROLER_TYPE + o) == C_COMP)
    {
        int* g = (int*)*((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER));
        WORD r = *((WORD*)((uintptr_t)g + S_RESOURCES));
        if (r < 5)r++;
        set_stat(g, r, S_RESOURCES);
    }
}

PROC g_proc_0042A466;
void briefing_check()
{
    *(byte*)(LEVEL_OBJ + 1) = 0;
    ((void (*)())g_proc_0042A466)();//original
}

PROC g_proc_00416930;
void player_race_mission_cheat()
{
    *(byte*)PLAYER_RACE = 0;//human = 0
    ((void (*)())g_proc_00416930)();//original
}

PROC g_proc_0042AC6D;
void player_race_mission_cheat2()
{
    ((void (*)())g_proc_0042AC6D)();//original
    *(byte*)PLAYER_RACE = 0;//human = 0
}

void sounds_ready_table_set(byte id, WORD snd)
{
    char buf[] = "\x0\x0";
    buf[0] = snd % 256;
    buf[1] = snd / 256;
    PATCH_SET((char*)(UNIT_SOUNDS_READY_TABLE + 2 * id), buf);
}

void show_message_from_tbl(int time, void* tbl, int str_id)
{
    char* msg = ((char* (*)(void*, int))F_GET_LINE_FROM_TBL)(tbl, str_id);
    show_message(time, msg);
}

char buttons_p[9 * BUTTON_SIZE + 1];

int empty_runes_p(byte)
{
    return *(byte*)LEVEL_OBJ == LVL_ORC13;
}

void buttons_init_p()
{
    int a = BUTTONS_CARDS + 8 * U_PEASANT + 4;
    a = *(int*)a;
    for (int i = 0; i < BUTTON_SIZE * 8; i++)
    {
        buttons_p[i] = *(byte*)(a + i);
    }
    
    int (*rr) (byte) = empty_runes_p;
    void (*r1) (int) = empty_cast_spell;

    buttons_p[BUTTON_SIZE * 8 + 0] = '\x8';//button id?
    buttons_p[BUTTON_SIZE * 8 + 1] = '\x0';//button id?
    buttons_p[BUTTON_SIZE * 8 + 2] = '\xB7';//icon
    buttons_p[BUTTON_SIZE * 8 + 3] = '\x0';//icon
    patch_setdword((DWORD*)(buttons_p + (BUTTON_SIZE * 8 + 4)), (DWORD)rr);
    patch_setdword((DWORD*)(buttons_p + (BUTTON_SIZE * 8 + 8)), (DWORD)r1);
    buttons_p[BUTTON_SIZE * 8 + 12] = '\x0';//arg
    buttons_p[BUTTON_SIZE * 8 + 13] = ORDER_SPELL_RUNES;//unit id
    buttons_p[BUTTON_SIZE * 8 + 14] = '\x95';//string from tbl
    buttons_p[BUTTON_SIZE * 8 + 15] = '\x1';//string from tbl
    buttons_p[BUTTON_SIZE * 8 + 16] = '\x0';//flags?
    buttons_p[BUTTON_SIZE * 8 + 17] = '\x0';//flags?
    buttons_p[BUTTON_SIZE * 8 + 18] = '\x0';//flags?
    buttons_p[BUTTON_SIZE * 8 + 19] = '\x0';//flags?

    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_PEASANT), (DWORD)9);
    patch_setdword((DWORD*)(BUTTONS_CARDS + 8 * U_PEASANT + 4), (DWORD)buttons_p);
}

void magic_runes(int* u)
{
    byte caster = *((byte*)((uintptr_t)u + S_ID));
    if (caster == U_PEASANT)
    {
        byte x = *((byte*)((uintptr_t)u + S_ORDER_X));
        byte y = *((byte*)((uintptr_t)u + S_ORDER_Y));
        if (unit_move(x, y, u))((void (*)(int*, byte))F_SPELL_SOUND_UNIT)(u, SS_EYE);
    }
    else((void (*)(int*))0x00443480)(u);//orig runes
    set_stat(u, ORDER_STOP, S_NEXT_ORDER);
}

void magic_actions_init()
{
    void (*r2) (int*) = magic_runes;
    char buf[] = "\x0\x0\x0\x0";
    patch_setdword((DWORD*)buf, (DWORD)r2);
    PATCH_SET((char*)(GF_SPELLS_FUNCTIONS + 4 * ORDER_SPELL_RUNES), buf);
}

//write your custom victory functions here
//-------------------------------------------------------------------------------
void v_human1(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
    else
    {
        //your custom victory conditions
        find_all_alive_units(U_ZULJIN);
        if (units == 0)lose(true);
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_CRITTER);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            kill_all();
            find_all_alive_units(U_OTRANSPORT);
            set_stat_all(S_HP, 5000);
            *(byte*)GB_HORSES = 1;
        }
        if (*(byte*)(GB_HORSES + 1) < 10)
        {
            *(byte*)(GB_HORSES + 1) = *(byte*)(GB_HORSES + 1) + 1;
        }
        if (*(byte*)(GB_HORSES + 1) >= 10)
        {
            WORD xx = 111;
            WORD yy = 106;
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
            if (units != 0)
            {
                xx = *((WORD*)((uintptr_t)unit[0] + S_X));
                yy = *((WORD*)((uintptr_t)unit[0] + S_Y));
            }
            find_all_alive_units(U_SKELETON);
            sort_stat(S_COLOR, P_ORANGE, CMP_EQ);
            if (units < 6)
            {
                find_all_alive_units(U_TEMPLE);
                sort_stat(S_COLOR, P_ORANGE, CMP_EQ);
                unit_create(118, 15, U_SKELETON, P_ORANGE, 1);
            }
            else
            {
                for (int i = 0; i < units; i++)
                {
                    struct GPOINT
                    {
                        WORD x;
                        WORD y;
                    } l;
                    l.x = xx;
                    l.y = yy;
                    ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                }
            }
            *(byte*)(GB_HORSES + 1) = 0;
        }
        find_all_alive_units(U_TEMPLE);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_NEQ);
        if (units == 0)
        {
            find_all_alive_units(U_RUNESTONE);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_NEQ);
            if (units == 0)
            {
                find_all_alive_units(U_DK);
                sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_NEQ);
                if (units == 0)
                {
                    win(true);
                }
            }
        }
	}
}

void v_human2(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_HTRANSPORT);
            sort_stat(S_COLOR, P_WHITE, CMP_EQ);
            if (units >= 1)
            {
                find_all_alive_units(U_KNIGHT);
                sort_stat(S_COLOR, P_WHITE, CMP_EQ);
                for (int i = 0; i < units; i++)
                {
                    struct GPOINT
                    {
                        WORD x;
                        WORD y;
                    } l;
                    l.x = 90;
                    l.y = 50;
                    ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                }
                *(byte*)GB_HORSES = 1;
            } 
        }
	}
}

void v_human3(bool rep_init)
{
    if (rep_init)
    {
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_PEON);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
            set_stat_all(S_COLOR, P_RED);
            *(byte*)GB_HORSES = 1;
        }
        find_all_alive_units(U_PEON);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_human4(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        find_all_alive_units(U_ZULJIN);
        if (units == 0)lose(true);
        find_all_alive_units(U_DRAGON);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_NEQ);
        if (units == 0)
        {
            find_all_alive_units(U_DRAGONROOST);
            sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_NEQ);
            if (units == 0)
            {
                win(true);
            }
        }
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_STRONGHOLD);
            sort_stat(S_COLOR, P_BLUE, CMP_EQ);
            if (units >= 1)
            {
                WORD xx = 90;
                WORD yy = 50;
                find_all_alive_units(ANY_MEN);
                sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
                sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
                if (units != 0)
                {
                    xx = *((WORD*)((uintptr_t)unit[0] + S_X));
                    yy = *((WORD*)((uintptr_t)unit[0] + S_Y));
                }
                find_all_alive_units(U_OGRE);
                sort_stat(S_COLOR, P_BLUE, CMP_EQ);
                for (int i = 0; i < units; i++)
                {
                    struct GPOINT
                    {
                        WORD x;
                        WORD y;
                    } l;
                    l.x = xx;
                    l.y = yy;
                    ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                }
                *(byte*)GB_HORSES = 1;
            }
        }
	}
}

void v_human5(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_ALLERIA);
        if (units == 0)lose(true);
	}
}

void v_human6(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        comps_vision(true);
        viz(P_RED, P_YELLOW, 1);
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_ALLERIA);
        if (units == 0)lose(true);
        find_all_alive_units(U_HADGAR);
        if (units == 0)lose(true);
        find_all_alive_units(U_DANATH);
        if (units == 0)lose(true);
        find_all_alive_units(U_TYRALYON);
        if (units == 0)lose(true);
        set_region(84, 34, 85, 35);
        find_all_alive_units(U_ALLERIA);
        sort_in_region();
        if (units != 0)
        {
            find_all_alive_units(U_HADGAR);
            sort_in_region();
            if (units != 0)
            {
                find_all_alive_units(U_DANATH);
                sort_in_region();
                if (units != 0)
                {
                    find_all_alive_units(U_TYRALYON);
                    sort_in_region();
                    if (units != 0)
                    {
                        win(true);
                    }
                }
            }
        }
    }
}

void v_human7(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_HADGAR);
        if (units == 0)lose(true);
        set_region(75, 69, 76, 70);
        sort_in_region();
        if (units != 0)win(true);
        
    }
}

void v_human8(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_TERON);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        if (units == 0)lose(true);
        find_all_alive_units(U_RUNESTONE);
        if (units == 0)lose(true);
        find_all_alive_units(U_SKELETON);
        sort_stat(S_OWNER, P_BLACK, CMP_EQ);
        sort_preplaced();
        if (units < 10)
        {
            find_all_alive_units(U_TEMPLE);
            sort_stat(S_OWNER, P_BLACK, CMP_EQ);
            for (int i = 0; i < units; i++)
            {
                building_start_build(unit[i], U_SKELETON, 0);
            }
        }
        find_all_alive_units(U_DEMON);
        sort_stat(S_OWNER, P_BLACK, CMP_EQ);
        sort_preplaced();
        if (units < 15)
        {
            find_all_alive_units(U_TEMPLE);
            sort_stat(S_OWNER, P_BLACK, CMP_EQ);
            for (int i = 0; i < units; i++)
            {
                building_start_build(unit[i], U_DEMON, 0);
            }
        }
    }
}

void v_human9(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_human10(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_HADGAR);
        if (units == 0)lose(true);
        comps_vision(true);
        viz(P_VIOLET, P_GREEN, 1);
        ally(P_VIOLET, P_GREEN, 1);
        for (int i = 0; i <= P_YELLOW; i++)
        {
            if ((i != P_VIOLET) && (i != P_GREEN))
            {
                viz(i, P_GREEN, 0);
                ally(i, P_GREEN, 0);
            }
        }
        WORD xx = 0;
        WORD yy = 0;
        find_all_alive_units(U_HADGAR);
        if (units != 0)
        {
            xx = *((WORD*)((uintptr_t)unit[0] + S_X));
            yy = *((WORD*)((uintptr_t)unit[0] + S_Y));
        }
        find_all_alive_units(U_BATTLESHIP);
        for (int i = 0; i < units; i++)
        {
            struct GPOINT
            {
                WORD x;
                WORD y;
            } l;
            l.x = xx;
            l.y = yy;
            ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
        }
        find_all_alive_units(U_HDESTROYER);
        for (int i = 0; i < units; i++)
        {
            struct GPOINT
            {
                WORD x;
                WORD y;
            } l;
            l.x = xx;
            l.y = yy;
            ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
        }
        xx = 59;
        yy = 80;
        find_all_alive_units(U_UTER);
        if (units != 0)
        {
            xx = *((WORD*)((uintptr_t)unit[0] + S_X));
            yy = *((WORD*)((uintptr_t)unit[0] + S_Y));
        }
        if ((get_val(ACTIVE_GRUNT, P_GREEN) > 2) && (get_val(ACTIVE_KNIGHT, P_GREEN) > 1))
        {
            find_all_alive_units(U_KNIGHT);
            sort_stat(S_OWNER, P_GREEN, CMP_EQ);
            sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_NEQ);
            for (int i = 0; i < units; i++)
            {
                struct GPOINT
                {
                    WORD x;
                    WORD y;
                } l;
                l.x = xx;
                l.y = yy;
                ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
            }
            find_all_alive_units(U_FOOTMAN);
            sort_stat(S_OWNER, P_GREEN, CMP_EQ);
            sort_stat(S_AI_ORDER, AI_ORDER_ATTACK, CMP_NEQ);
            for (int i = 0; i < units; i++)
            {
                struct GPOINT
                {
                    WORD x;
                    WORD y;
                } l;
                l.x = xx;
                l.y = yy;
                ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
            }
        }
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_UTER);
            if (units == 0)
            {
                xx = 116;
                yy = 70;
                find_all_alive_units(ANY_MEN);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                sort_stat(S_ID, U_PEASANT, CMP_NEQ);
                for (int i = 0; i < units; i++)
                {
                    struct GPOINT
                    {
                        WORD x;
                        WORD y;
                    } l;
                    l.x = xx;
                    l.y = yy;
                    ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
                }
                *(byte*)GB_HORSES = 1;
            }
        }
	}
}

void v_human11(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        ua[0] = U_TYRALYON;
        ut[0] = U_TERON;
        ua[1] = U_DEATHWING;
        ut[1] = U_DEATHWING;
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        find_all_alive_units(U_TYRALYON);
        if (units == 0)lose(true);
        find_all_alive_units(U_DEATHWING);
        if (units == 0)
        {
            find_all_alive_units(U_TERON);
            if (units == 0)win(true);
        }
	}
}

void v_human12(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_CRITTER);
        if (units == 0)win(true);
	}
}

void v_human13(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        comps_vision(true);
        viz(P_YELLOW, P_WHITE, 1);
        find_all_alive_units(U_CHOGAL);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        if (units < 8)lose(true);
        set_region(117, 126, 120, 127);
        sort_in_region();
        if (units == 8)win(true);
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_CHOGAL);
            give_all(P_YELLOW);
            *(byte*)GB_HORSES = 1;
        }
	}
}

void v_human14(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        if (!check_opponents(*(byte*)LOCAL_PLAYER))
        {
            //*(byte*)LEVEL_OBJ = 254 + LVL_ORC1;
            //*(WORD*)LEVEL_ID = 0x52C7;
            //*(byte*)LEVEL_OBJ = LVL_ORC2;
            //*(WORD*)LEVEL_ID = 0x52CB;
            win(true);
        }
	}
}

void v_xhuman1(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_TYRALYON);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_ALLERIA);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DANATH);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_KURDRAN);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_KARGATH);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman2(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman3(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_KARGATH);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_PORTAL);
        sort_stat(S_COLOR, P_YELLOW, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman4(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman5(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_KARGATH);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman6(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TYRALYON);
        sort_stat(S_COLOR, P_BLUE, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman7(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_KARGATH);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_RED, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman8(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        if (cmp_res(*(byte*)LOCAL_PLAYER, 0, 0xC3, 0x50, 0, 0, CMP_BIGGER_EQ))call_default_kill();
        find_all_alive_units(U_CHOGAL);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GROM);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman9(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        ua[0] = U_DEMON;
        ut[0] = U_HADGAR;
        call_default_kill();
        find_all_alive_units(U_RUNESTONE);
        sort_stat(S_COLOR, P_GREEN, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_PORTAL);
        sort_stat(S_COLOR, P_GREEN, CMP_EQ);
        if (units == 0)lose(true);
        if (*(byte*)GB_HORSES == 0)
        {
            unit_create(93, 92, U_ALTAR, P_ORANGE, 1);
            unit_create(92, 92, U_DEMON, P_ORANGE, 3);
            unit_create(92, 92, U_ALLERIA, P_ORANGE, 6);
            *(byte*)GB_HORSES = 1;
        }
        find_all_alive_units(U_DEMON);
        sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
        sort_preplaced();
        if (units < 15)
        {
            find_all_alive_units(U_ALTAR);
            sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
            for (int i = 0; i < units; i++)
            {
                building_start_build(unit[i], U_DEMON, 0);
            }
        }
        find_all_alive_units(U_DEMON);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        if (units < 3)
        {
            find_all_alive_units(U_PORTAL);
            for (int i = 0; i < units; i++)
            {
                building_start_build(unit[i], U_DEMON, 0);
            }
        }
    }
}

void v_xhuman10(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_ORANGE, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman11(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_DEATHWING);
        sort_stat(S_COLOR, P_VIOLET, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DANATH);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_ALLERIA);
        sort_stat(S_COLOR, P_BLACK, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_YELLOW, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_TERON);
        sort_stat(S_COLOR, P_YELLOW, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_xhuman12(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        allow_table(P_GREEN, 4, L_SLOW, 1);
        allow_table(P_GREEN, 4, L_INVIS, 1);
        allow_table(P_GREEN, 4, L_BLIZZARD, 1);

        if (!check_opponents(*(byte*)LOCAL_PLAYER))
        {
            //*(byte*)LEVEL_OBJ = LVL_XORC1 - 2;
            //*(WORD*)LEVEL_ID = 0x53C7 - 2;
            win(true);
        }

        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_ZULJIN);
        sort_stat(S_COLOR, P_BLUE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_DENTARG);
        sort_stat(S_COLOR, P_BLUE, CMP_EQ);
        if (units == 0)lose(true);
        find_all_alive_units(U_GULDAN);
        sort_stat(S_COLOR, P_BLUE, CMP_EQ);
        if (units == 0)lose(true);
    }
}

void v_orc1(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_orc2(bool rep_init)
{
    if (rep_init)
    {
        //ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        call_default_kill();
    }
}

void v_orc3(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        if (!slot_alive(P_GREEN))win(true);
        comps_vision(true);
        viz(P_BLACK, P_ORANGE, 1);
        viz(P_BLACK, P_BLUE, 1);
        if (*(byte*)GB_HORSES == 3)
        {
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
            for (int i = 0; i < units; i++)
            {
                struct GPOINT
                {
                    WORD x;
                    WORD y;
                } l;
                l.x = 56;
                l.y = 61;
                ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
            }
            *(byte*)GB_HORSES = 4;
        }
        if (*(byte*)GB_HORSES == 2)
        {
            *(byte*)GB_HORSES = 3;
        }
        if (*(byte*)GB_HORSES == 1)
        {
            center_view(24, 8);
            bullet_create(34 * 32 + 16, 11 * 32 + 16, B_CAT_HIT);
            tile_remove_walls(34, 11);
            bullet_create(34 * 32 + 16, 12 * 32 + 16, B_CAT_HIT);
            tile_remove_walls(34, 12);
            bullet_create(34 * 32 + 16, 13 * 32 + 16, B_CAT_HIT);
            tile_remove_walls(34, 13);
            bullet_create(34 * 32 + 16, 14 * 32 + 16, B_CAT_HIT);
            tile_remove_walls(34, 14);
            bullet_create(34 * 32 + 16, 15 * 32 + 16, B_CAT_HIT);
            tile_remove_walls(34, 15);
            *(byte*)GB_HORSES = 2;
        }
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_HLUMBER);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            set_stat_all(S_HP, 50);
            *(byte*)GB_HORSES = 1;
        }
        if (*(byte*)(GB_HORSES + 1) == 1)
        {
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, P_RED, CMP_EQ);
            if (units == 0)
            {
                find_all_alive_units(ANY_MEN);
                sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
                give_all(P_BLACK);
                find_all_alive_units(ANY_MEN);
                sort_stat(S_OWNER, P_BLUE, CMP_EQ);
                give_all(P_BLACK);
                *(byte*)(GB_HORSES + 1) = 2;
            }
        }
        if (*(byte*)(GB_HORSES + 1) == 0)
        {
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
            set_region(47, 54, 55, 62);
            sort_in_region();
            if (units != 0)
            {
                ally(P_RED, P_BLUE, 0);
                bullet_create(56 * 32 + 16, 62 * 32 + 16, B_CAT_HIT);
                tile_remove_walls(56, 62);
                bullet_create(57 * 32 + 16, 62 * 32 + 16, B_CAT_HIT);
                tile_remove_walls(57, 62);
                bullet_create(58 * 32 + 16, 62 * 32 + 16, B_CAT_HIT);
                tile_remove_walls(58, 62);
                bullet_create(59 * 32 + 16, 62 * 32 + 16, B_CAT_HIT);
                tile_remove_walls(59, 62);
                *(byte*)(GB_HORSES + 1) = 1;
            }
        }
    }
}

void v_orc4(bool rep_init)
{
    if (rep_init)
    {
        ai_fix_plugin(true);
        saveload_fixed = true;
        //your initialize
    }
    else
    {
        //your custom victory conditions
        find_all_alive_units(U_CASTLE);
        sort_stat(S_OWNER, *(byte*)LOCAL_PLAYER, CMP_EQ);
        if (units == 0)lose(true);
        if (!slot_alive(P_VIOLET))win(true);
    }
}

void v_orc5(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        comps_vision(true);
        viz(P_YELLOW, P_BLUE, 1);
        find_all_alive_units(U_LOTHAR);
        if (units == 0)lose(true);
        find_all_alive_units(U_UTER);
        if (units == 0)win(true);
	}
}

void v_orc6(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        ua[0] = U_GROM;
        ut[0] = U_KARGATH;
        find_all_alive_units(U_GROM);
        if (units == 0)lose(true);
        find_all_alive_units(U_KARGATH);
        if (units == 0)win(true);
	}
}

void v_orc7(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (get_val(RUNESTONE, P_RED) == 0)win(true);
        call_default_kill();
	}
}

void v_orc8(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        find_all_alive_units(U_LOTHAR);
        if (units == 0)lose(true);
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        set_region(17, 1, 18, 2);
        find_all_alive_units(U_UTER);
        sort_in_region();
        if (units != 0)
        {
            find_all_alive_units(U_LOTHAR);
            sort_in_region();
            if (units != 0)
            {
                if (get_val(PORTAL, P_VIOLET) == 0)win(true);
            }
        }

        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, P_ORANGE, CMP_EQ);
            sort_stat(S_MOVEMENT_TYPE, MOV_LAND, CMP_EQ);
            sort_stat(S_Y, 10, CMP_SMALLER);
            for (int i = 0; i < units; i++)
            {
                struct GPOINT
                {
                    WORD x;
                    WORD y;
                } l;
                l.x = 111;
                l.y = 116;
                ((int (*)(int*, int, GPOINT*))F_ICE_SET_AI_ORDER)(unit[i], AI_ORDER_ATTACK, &l);
            }
            *(byte*)GB_HORSES = 1;
        }
	}
}

void v_orc9(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

char portal_name[] = "BRUH\\portal.wav\x0";
void* portal_sound = NULL;

void v_orc10(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        ut[0] = U_HADGAR;
        ua[0] = U_PORTAL;
        ally(P_BLACK, P_RED, 0);
        ally(P_BLACK, P_BLUE, 0);
        ally(P_BLACK, P_GREEN, 0);
        ally(P_BLACK, P_ORANGE, 0);
        ally(P_BLACK, P_WHITE, 0);
        ally(P_BLACK, P_YELLOW, 0);
        comps_vision(true);
        ally(P_BLACK, P_VIOLET, 1);
        viz(P_BLACK, P_VIOLET, 1);
        if (*(byte*)GB_HORSES == 3)
        {
            win(true);
        }
        if (*(byte*)GB_HORSES > 0)
        {
            *(byte*)GB_HORSES = *(byte*)GB_HORSES + 1;
        }
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_PORTAL);
        if (units == 0)lose(true);
        if (*(byte*)GB_HORSES == 0)
        {
            find_all_alive_units(U_HADGAR);
            set_region(12, 9, 19, 16);
            sort_in_region();
            if (units != 0)
            {
                center_view(10, 7);
                kill_all();
                char msg[] = "Khadgar was sucked into portal!";
                show_message(20, msg);
                sound_play_from_file(0, (DWORD)portal_name, portal_sound);
                *(byte*)GB_HORSES = 1;
            }
        }
	}
}

void v_orc11(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_orc12(bool rep_init)
{
	if (rep_init)
	{
		//your initialize
	}
	else
	{
		//your custom victory conditions
        ua[0] = U_FARM;
        ut[0] = U_PEON;
        find_all_alive_units(U_PEON);
        if (units == 0)win(true);
	}
}

void v_orc13(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
        ut[0] = U_PEASANT;
        ua[0] = U_PEASANT;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        comps_vision(true);
        viz(P_RED, P_BLUE, 1);
        ut[0] = U_PEASANT;
        ua[0] = U_PEASANT;
        manacost(RUNES, 1);
        call_default_kill();
        if (*(byte*)GB_HORSES == 3)
        {
            ut[0] = 255;
            ua[0] = 255;
        }
        if (*(byte*)GB_HORSES == 2)
        {
            ut[0] = 255;
            ua[0] = 255;
            find_all_alive_units(ANY_MEN);
            sort_stat(S_OWNER, P_BLUE, CMP_EQ);
            set_stat_all(S_ATTACKER_POINTER, 0);
            set_stat_all(S_ATTACKER_POINTER + 1, 0);
            set_stat_all(S_ATTACKER_POINTER + 2, 0);
            set_stat_all(S_ATTACKER_POINTER + 3, 0);
            *(byte*)GB_HORSES = 3;
        }
        if (*(byte*)GB_HORSES == 1)
        {
            *(byte*)GB_HORSES = 2;
        }
        if (*(byte*)GB_HORSES == 0)
        {
            *(byte*)GB_HORSES = 1;
        }
	}
}

void v_orc14(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        byte l = *(byte*)LOCAL_PLAYER;
        if (!slot_alive(l))lose(true);
        else
        {
            if (!check_opponents(l))
            {
                //*(byte*)LEVEL_OBJ = LVL_XHUMAN1 - 2;
                //*(WORD*)LEVEL_ID = 0x53C6 - 2;
                win(true);
            }
        }
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        find_all_alive_units(U_LOTHAR);
        if (units == 0)lose(true);
	}
}

void v_xorc1(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_xorc2(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        comps_vision(true);
        viz(P_BLUE, P_GREEN, 1);
        call_default_kill();
	}
}

void v_xorc3(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        find_all_alive_units(U_LOTHAR);
        if (units == 0)lose(true);
        set_region(14, 69, 15, 70);
        sort_in_region();
        if (units != 0)win(true);
	}
}

void v_xorc4(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_xorc5(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        bool f = true;
        for (int i = P_GREEN; i <= P_WHITE; i++)if (slot_alive(i))f = false;
        if (f)win(true);
        if (*(byte*)GB_HORSES == 1)lose(true);//killed blue unit (unit kill deselect)
	}
}

void v_xorc6(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
	}
}

void v_xorc7(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        if ((get_val(ALL_UNITS, P_RED) >= 250) && (get_val(FOOD_LIMIT, P_RED) >= 200))win(true);
	}
}

void v_xorc8(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_PEASANT);
        sort_stat(S_COLOR, P_WHITE, CMP_EQ);
        if (units < 25)lose(true);

        find_all_alive_units(U_GOBLINS);
        sort_stat(S_OWNER, P_GREEN, CMP_EQ);
        sort_preplaced();
        if (units < 3)
        {
            find_all_alive_units(U_ALCHEMIST);
            sort_stat(S_OWNER, P_GREEN, CMP_EQ);
            for (int i = 0; i < units; i++)
            {
                building_start_build(unit[i], U_GOBLINS, 0);
            }
        }
	}
}

void v_xorc9(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        if (!slot_alive(*(byte*)LOCAL_PLAYER))lose(true);
        if (get_val(TH3, P_BLUE) == 0)win(true);
	}
}

void v_xorc10(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
	}
}

void v_xorc11(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        find_all_alive_units(U_UTER);
        if (units == 0)lose(true);
        find_all_alive_units(U_HADGAR);
        if (units == 0)lose(true);
	}
}

void v_xorc12(bool rep_init)
{
	if (rep_init)
	{
        ai_fix_plugin(true);
        saveload_fixed = true;
		//your initialize
	}
	else
	{
		//your custom victory conditions
        call_default_kill();
        ally(P_BLUE, P_GREEN, 0);
        ally(P_BLUE, P_VIOLET, 0);
        ally(P_VIOLET, P_GREEN, 0);
	}
}

void v_custom(bool rep_init)
{
	if (rep_init)
	{
		//pathfind_fix(true);
		//ai_fix_plugin(true);
		//your initialize
	}
	else
	{
		//your custom victory conditions
	}
}
//-------------------------------------------------------------------------------

bool cheat_win = false;

void (*triggers[])(bool) = { v_human1,v_orc1,v_human2,v_orc2,v_human3,v_orc3,v_human4,v_orc4,v_human5,v_orc5,v_human6,v_orc6,v_human7,v_orc7,v_human8,v_orc8,v_human9,v_orc9,v_human10,v_orc10,v_human11,v_orc11,v_human12,v_orc12,v_human13,v_orc13,v_human14,v_orc14,v_xhuman1,v_xorc1,v_xhuman2,v_xorc2,v_xhuman3,v_xorc3,v_xhuman4,v_xorc4,v_xhuman5,v_xorc5,v_xhuman6,v_xorc6,v_xhuman7,v_xorc7,v_xhuman8,v_xorc8,v_xhuman9,v_xorc9,v_xhuman10,v_xorc10,v_xhuman11,v_xorc11,v_xhuman12,v_xorc12 };

void trig()
{
    byte lvl = *(byte*)LEVEL_OBJ;
    if (a_custom)
    {
        v_custom(false);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(false);
        else
            v_custom(false);
    }
	first_step = false;
    if (cheat_win)
    {
        /*
        if (lvl == LVL_HUMAN14)
        {
            *(byte*)LEVEL_OBJ = 254 + LVL_ORC1;
            *(WORD*)LEVEL_ID = 0x52C7;
        }
        else if (lvl == LVL_ORC14)
        {
            *(byte*)LEVEL_OBJ = LVL_XHUMAN1 - 2;
            *(WORD*)LEVEL_ID = 0x53C6 - 2;
        }
        else if (lvl == LVL_XHUMAN12)
        {
            *(byte*)LEVEL_OBJ = LVL_XORC1 - 2;
            *(WORD*)LEVEL_ID = 0x53C7 - 2;
        }
        */
        win(true);
    }
}

void trig_init()
{
    cheat_win = false;
	first_step = true;
    byte lvl = *(byte*)LEVEL_OBJ;
    if (a_custom)
    {
        v_custom(true);
    }
    else
    {
        if ((lvl >= 0) && (lvl < 52))
            ((void (*)(bool))triggers[lvl])(true);
        else
            v_custom(true);
    }
}

void replace_def()
{
    //set all vars to default
    memset(ua, 255, sizeof(ua));
    memset(ut, 255, sizeof(ut));
	memset(vizs_areas, 0, sizeof(vizs_areas));
	vizs_n = 0;
    ai_fixed = false;
	saveload_fixed = false;
    A_portal = false;
}

void replace_common()
{
    //peon can build any buildings
    char ballbuildings[] = "\x0\x0";//d1 05
    PATCH_SET((char*)BUILD_ALL_BUILDINGS1, ballbuildings);
    char ballbuildings2[] = "\x0";//0a
    PATCH_SET((char*)BUILD_ALL_BUILDINGS2, ballbuildings2);
    char ballbuildings3[] = "\x0";//68
    PATCH_SET((char*)BUILD_ALL_BUILDINGS3, ballbuildings3);

    //any building can train any unit
    char ballunits[] = "\xeb";//0x74
    PATCH_SET((char*)BUILD_ALL_UNITS1, ballunits);
    char ballunits2[] = "\xA1\xBC\x47\x49\x0\x90\x90";//8b 04 85 bc 47 49 00
    PATCH_SET((char*)BUILD_ALL_UNITS2, ballunits2);

    //any building can make any research
    char allres[] = "\xB1\x1\x90";
    PATCH_SET((char*)BUILD_ALL_RESEARCH1, allres);
    PATCH_SET((char*)BUILD_ALL_RESEARCH2, allres);

    //allow all units cast all spells
    char allsp[] = "\x90\x90";
    PATCH_SET((char*)CAST_ALL_SPELLS, allsp);

    //show kills
    byte d = S_KILLS;
    char sdmg[] = "\x8a\x90\x82\x0\x0\x0\x8b\xfa";//units
    sdmg[2] = d;
    PATCH_SET((char*)SPEED_STAT_UNITS, sdmg);
    char sdmg2[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90";//catas
    sdmg2[2] = d;
    PATCH_SET((char*)SPEED_STAT_CATS, sdmg2);
    char sdmg3[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90";//archers
    sdmg3[2] = d;
    PATCH_SET((char*)SPEED_STAT_ARCHERS, sdmg3);
    char sdmg4[] = "\x8a\x82\x82\x0\x0\x0\x90\x90\x90";//berserkers
    sdmg4[2] = d;
    PATCH_SET((char*)SPEED_STAT_BERSERKERS, sdmg4);
    char sdmg5[] = "\x8a\x88\x82\x0\x0\x0\x90\x90\x90";//ships
    sdmg5[2] = d;
    PATCH_SET((char*)SPEED_STAT_SHIPS, sdmg5);

    char dmg_fix[] = "\xeb";
    PATCH_SET((char*)DMG_FIX, dmg_fix);

    draw_stats_fix(true);
}

void replace_back()
{
    //replace all to default
    comps_vision(false);
    repair_cat(false);
    trigger_time('\xc8');
    upgr(SWORDS, 2);
    upgr(ARMOR, 2);
    upgr(ARROWS, 1);
    upgr(SHIP_DMG, 5);
    upgr(SHIP_ARMOR, 5);
    upgr(CATA_DMG, 15);
    manacost(VISION, 70);
    manacost(HEAL, 6);
    manacost(GREATER_HEAL, 5);
    manacost(EXORCISM, 4);
    manacost(FIREBALL, 100);
    manacost(FLAME_SHIELD, 80);
    manacost(SLOW, 50);
    manacost(INVIS, 200);
    manacost(POLYMORPH, 200);
    manacost(BLIZZARD, 25);
    manacost(EYE_OF_KILROG, 70);
    manacost(BLOOD, 50);
    manacost(RAISE_DEAD, 50);
    manacost(COIL, 100);
    manacost(WHIRLWIND, 100);
    manacost(HASTE, 50);
    manacost(UNHOLY_ARMOR, 100);
    manacost(RUNES, 200);
    manacost(DEATH_AND_DECAY, 25);
    draw_stats_fix(false);
	ai_fix_plugin(false);
}

void replace_trigger()
{
    replace_back();
    replace_def();
    replace_common();

    buttons_init_p();
    magic_actions_init();

    //replace original victory trigger
    char trig_jmp[] = "\x74\x1A";//74 0F
    PATCH_SET((char*)VICTORY_JMP, trig_jmp);
    char rep[] = "\xc7\x05\x38\x0d\x4c\x0\x30\x8C\x45\x0";
    void (*repf) () = trig;
    patch_setdword((DWORD*)(rep + 6), (DWORD)repf);
    PATCH_SET((char*)VICTORY_TRIGGER, rep);
	trig_init();
}

char msg_written_text[256];
char msg_written_text_orig[256];

PROC g_proc_0044CEBB;
void get_msg(int a, char* text)
{
    //this function called when you write in game chat and press enter
    ((void (*)(int, char*))g_proc_0044CEBB)(a, text);//original
    for (int i = 0; i < 60; i++)
    {
        msg_written_text[i] = text[i];
    }
    char* token = strtok(msg_written_text, " ");
    if (token != NULL)
    {
        if (lstrcmpi(token, "/win") == 0)
        {
            cheat_win = true;
        }
    }
}

PROC g_proc_0042A4A1;
int new_game(int a, int b, long c)
{
    if (remember_music != 101)
        *(DWORD*)VOLUME_MUSIC = remember_music;
    if (remember_sound != 101)
        *(DWORD*)VOLUME_SOUND = remember_sound;
    ((void (*)(DWORD))F_SET_VOLUME)(SET_VOLUME_PARAM);//set volume
    remember_music = 101;
    remember_sound = 101;

    a_custom = b % 256;//custom game or campaign
	if (a_custom)*(byte*)LEVEL_OBJ=53;//remember custom obj
	else
	{
	if (*(byte*)LEVEL_OBJ==53)a_custom=1;//fix for when saved game loads custom get broken
	}
    replace_trigger();
    memset((void*)GB_HORSES, 0, 16 * sizeof(byte));
    int original = ((int (*)(int, int, long))g_proc_0042A4A1)(a, b, c);//original
    game_started = true;
    return original;
}

PROC g_proc_0041F7E4;
int load_game(int a)
{
    int original = ((int (*)(int))g_proc_0041F7E4)(a);//original
    replace_trigger();

    return original;
}

void drawing()
{
    ((void (*)(int, int, int, int))F_INVALIDATE)(0, 0, m_screen_w, m_screen_h);
}

PROC g_proc_00421F57;
void draw_hook3()
{
    ((void (*)())g_proc_00421F57)();//original
    drawing();
}

void hook(int adr, PROC* p, char* func)
{
    *p = patch_call((char*)adr, func);
}

void common_hooks()
{
    hook(0x0045271B, &g_proc_0045271B, (char*)update_spells);
    hook(0x004522B9, &g_proc_004522B9, (char*)seq_run);

    hook(0x0041038E, &g_proc_0041038E, (char*)damage1);
    hook(0x00409F3B, &g_proc_00409F3B, (char*)damage2);
    hook(0x0040AF70, &g_proc_0040AF70, (char*)damage3);
    hook(0x0040AF99, &g_proc_0040AF99, (char*)damage4);
    hook(0x00410762, &g_proc_00410762, (char*)damage5);
    hook(0x004428AD, &g_proc_004428AD, (char*)damage6);

    hook(0x0040DF71, &g_proc_0040DF71, (char*)bld_unit_create);
    hook(0x00451728, &g_proc_00451728, (char*)unit_kill_deselect);
    hook(0x00452939, &g_proc_00452939, (char*)grow_struct_count_tables);
    hook(0x0042479E, &g_proc_0042479E, (char*)peon_into_goldmine);
    hook(0x00424745, &g_proc_00424745, (char*)goods_into_inventory);
    hook(0x004529C0, &g_proc_004529C0, (char*)goods_into_inventory);
	hook(0x00451054, &g_proc_00451054, (char*)count_add_to_tables_load_game);
    hook(0x00438A5C, &g_proc_00438A5C, (char*)unset_peon_ai_flags);
    hook(0x00438985, &g_proc_00438985, (char*)unset_peon_ai_flags);
	
	hook(0x0040EEDD, &g_proc_0040EEDD, (char*)upgrade_tower);
    hook(0x00442E25, &g_proc_00442E25, (char*)create_skeleton);
    hook(0x00425D1C, &g_proc_00425D1C, (char*)cast_raise);
    hook(0x0042757E, &g_proc_0042757E, (char*)ai_spell);
    hook(0x00427FAE, &g_proc_00427FAE, (char*)ai_attack);
    hook(0x00427FFF, &g_proc_00427FFF, (char*)ai_attack_nearest_enemy);
	
    hook(0x0044CEBB, &g_proc_0044CEBB, (char*)get_msg);

	hook(0x0042A4A1, &g_proc_0042A4A1, (char*)new_game);
	hook(0x0041F7E4, &g_proc_0041F7E4, (char*)load_game);
}

void files_hooks()
{
    files_init();

    hook(0x004542FB, &g_proc_004542FB, (char*)grp_draw_cross);
    hook(0x00454DB4, &g_proc_00454DB4, (char*)grp_draw_bullet);
    hook(0x00454BCA, &g_proc_00454BCA, (char*)grp_draw_unit);
    hook(0x00455599, &g_proc_00455599, (char*)grp_draw_building);
    hook(0x0043AE54, &g_proc_0043AE54, (char*)grp_draw_building_placebox);
    hook(0x0044538D, &g_proc_0044538D, (char*)grp_portrait_init);
    hook(0x004453A7, &g_proc_004453A7, (char*)grp_draw_portrait);
    hook(0x004452B0, &g_proc_004452B0, (char*)grp_draw_portrait_buttons);
    hook(0x0044A65C, &g_proc_0044A65C, (char*)status_get_tbl);
    hook(0x0044AC83, &g_proc_0044AC83, (char*)unit_hover_get_id);
    hook(0x0044AE27, &g_proc_0044AE27, (char*)unit_hover_get_tbl);
    hook(0x0044AE56, &g_proc_0044AE56, (char*)button_description_get_tbl);
    hook(0x0044B23D, &g_proc_0044B23D, (char*)button_hotkey_get_tbl);
    hook(0x004354C8, &g_proc_004354C8, (char*)objct_get_tbl_custom);
    hook(0x004354FA, &g_proc_004354FA, (char*)objct_get_tbl_campanign);
    hook(0x004300A5, &g_proc_004300A5, (char*)objct_get_tbl_briefing_task);
    hook(0x004300CA, &g_proc_004300CA, (char*)objct_get_tbl_briefing_title);
    hook(0x004301CA, &g_proc_004301CA, (char*)objct_get_tbl_briefing_text);
    hook(0x00430113, &g_proc_00430113, (char*)objct_get_briefing_speech);
    hook(0x0041F0F5, &g_proc_0041F0F5, (char*)finale_get_tbl);
    hook(0x0041F1E8, &g_proc_0041F1E8, (char*)finale_credits_get_tbl);
    hook(0x0041F027, &g_proc_0041F027, (char*)finale_get_speech);
    hook(0x00417E33, &g_proc_00417E33, (char*)credits_small_get_tbl);
    hook(0x00417E4A, &g_proc_00417E4A, (char*)credits_big_get_tbl);
    hook(0x0042968A, &g_proc_0042968A, (char*)act_get_tbl_small);
    hook(0x004296A9, &g_proc_004296A9, (char*)act_get_tbl_big);
    hook(0x0041C51C, &g_proc_0041C51C, (char*)netstat_get_tbl_nation);
    hook(0x00431229, &g_proc_00431229, (char*)rank_get_tbl);
    hook(0x004372EE, &g_proc_004372EE, (char*)pcx_load_menu);
    hook(0x00430058, &g_proc_00430058, (char*)pcx_load_briefing);
    hook(0x00429625, &g_proc_00429625, (char*)pcx_load_act);
    hook(0x00429654, &g_proc_00429654, (char*)pcx_load_act);
    hook(0x0041F004, &g_proc_0041F004, (char*)pcx_load_final);
    hook(0x00417DDB, &g_proc_00417DDB, (char*)pcx_load_credits);
    hook(0x0043169E, &g_proc_0043169E, (char*)pcx_load_statistic);
    hook(0x00462D4D, &g_proc_00462D4D, (char*)storm_file_load);
    hook(0x0041F9FD, &g_proc_0041F9FD, (char*)tilesets_load);
    
    hook(0x0041F97D, &g_proc_0041F97D, (char*)map_file_load);

    hook(0x0042A443, &g_proc_0042A443, (char*)act_init);

    hook(0x0044F37D, &g_proc_0044F37D, (char*)main_menu_init);

    hook(0x0043B16F, &g_proc_0043B16F, (char*)smk_play_sprintf_name);
    hook(0x0043B362, &g_proc_0043B362, (char*)smk_play_sprintf_blizzard);

    hook(0x00440F4A, &g_proc_00440F4A, (char*)music_play_get_install);
    hook(0x00440F5F, &g_proc_00440F5F, (char*)music_play_get_install);
    patch_call((char*)0x00440F41, (char*)music_play_sprintf_name);
    char buf[] = "\x90";
    PATCH_SET((char*)(0x00440F41 + 5), buf);//7 bytes call

    hook(0x00422D76, &g_proc_00422D76, (char*)sound_play_unit_speech);
    hook(0x00422D5F, &g_proc_00422D5F, (char*)sound_play_unit_speech_soft);
    hook(0x00402811, &g_proc_00402811, (char*)button_play_sound);
}

void capture_fix()
{
    char buf[] = "\xB0\x01\xF6\xC1\x02\x74\x02\xB0\x02\x50\x66\x8B\x7B\x18\x66\x8B\x6B\x1A\x8B\xD7\x8B\xF5\x29\xC2\x29\xC6\x8D\x43\x27\x31\xC9\x89\x44\x24\x24\x8A\x08\xC1\xE1\x02\x66\x8B\x81\x1C\xEE\x4C\x00\x66\x8B\x89\x1E\xEE\x4C\x00\x66\x01\xF8\x66\x01\xE9\x5D\x01\xE8\x01\xE9\x90\x90";
    PATCH_SET((char*)CAPTURE_BUG, buf);
}

void fireshield_cast_fix()
{
    char buf[] = "\x90\x90";
    PATCH_SET((char*)FIRE_CAST1, buf);
    char buf2[] = "\x90\x90\x90\x90\x90\x90";
    PATCH_SET((char*)FIRE_CAST2, buf2);
}

char save_folder[] = "Save\\Bruh\\";

void change_save_folder()
{
    patch_setdword((DWORD*)0x0043C07A, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043CE66, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043D2C1, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043D383, (DWORD)save_folder);
    patch_setdword((DWORD*)0x0043FB66, (DWORD)save_folder);
}

bool dll_called = false;

extern "C" __declspec(dllexport) void w2p_init()
{
    DWORD check_exe = *(DWORD*)0x48F2F0;
    if (check_exe == 0x48555242)//BRUH
    {
        //if (!dll_called)
        {
            m_screen_w = *(WORD*)SCREEN_SIZE_W;
            m_screen_h = *(WORD*)SCREEN_SIZE_H;
            hook(0x00421F57, &g_proc_00421F57, (char*)draw_hook3);

            common_hooks();
            files_hooks();

            hook(0x0041F915, &g_proc_0041F915, (char*)map_load);
            hook(0x0042BB04, &g_proc_0042BB04, (char*)map_create_unit);
            hook(0x00418937, &g_proc_00418937, (char*)dispatch_die_unitdraw_update_1_man);

            hook(0x0042A466, &g_proc_0042A466, (char*)briefing_check);
            hook(0x00416930, &g_proc_00416930, (char*)player_race_mission_cheat);
            hook(0x0042AC6D, &g_proc_0042AC6D, (char*)player_race_mission_cheat2);
            

            //char buf[] = "\x90\x90\x90";
            //PATCH_SET((char*)0x0045158A, buf);//peon die with res sprite bug fix
            //hook(0x00451590, &g_proc_00451590, (char*)unit_kill_peon_change);

            //char buf_showpath[] = "\xB0\x1\x90\x90\x90";
            //PATCH_SET((char*)0x00416691, buf_showpath);
            //hook(0x0045614E, &g_proc_0045614E, (char*)receive_cheat_single);

            *(byte*)(0x0049D93C) = 0;//remove text
            *(byte*)(0x0049DC24) = 0;//from main menu
            patch_call((char*)0x004886B3, (char*)0x0048CCA2);//remove fois HD text from main menu

            //char buf_i[] = "\xE8\x1B\x31\x5\x0";
            //PATCH_SET((char*)0x00428E08, buf_i);//fix fois install

            change_save_folder();
            capture_fix();
            fireshield_cast_fix();

            char buf_unload_check[] = "\x0\x0\x0\x0";
            PATCH_SET((char*)0x0048F2F0, buf_unload_check);//dll unload

            dll_called = true;
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID)
{
    //DWORD check_exe = *(DWORD*)0x48F2F0;
    //if (check_exe == 0x4E4F5453)//STON
    //{
    //    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    //    {
    //        bool ex = false;
    //        FILE* file;
    //        if ((file = fopen("INSTALL.MPQ", "r")))
    //        {
    //            fclose(file);
    //            ex = true;
    //        }
    //        if (ex)
    //        {
    //            char mpq[] = "mpq";
    //            PATCH_SET((char*)(0x49DBFC + 9), mpq);//install exe/mpq name
    //        }
    //        w2p_init();
    //    }
    //}
    return TRUE;
}