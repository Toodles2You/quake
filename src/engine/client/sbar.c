/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023-2024 Justin Keller

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
===========================================================================
*/

#include "clientdef.h"

int sb_lines; // scan lines to draw

static bool sb_updated; // if >= vid.numpages, no update needed

#define sb_minus 10 // num frame for '-' stats digit
static qpic_t *sb_nums[2][11];
static qpic_t *sb_colon, *sb_slash;
static qpic_t *sb_ibar;
static qpic_t *sb_sbar;
static qpic_t *sb_scorebar;

static qpic_t *sb_weapons[7][8]; // 0 is active, 1 is owned, 2-5 are flashes
static qpic_t *sb_ammo[4];
static qpic_t *sb_sigil[4];
static qpic_t *sb_armor[3];
static qpic_t *sb_items[32];

static qpic_t *sb_faces[7][2]; // 0 is gibbed, 1 is dead, 2-6 are alive
							   // 0 is static, 1 is temporary animation
static qpic_t *sb_face_invis;
static qpic_t *sb_face_quad;
static qpic_t *sb_face_invuln;
static qpic_t *sb_face_invis_invuln;

static bool sb_showscores;

static qpic_t *rsb_invbar[2];
static qpic_t *rsb_weapons[5];
static qpic_t *rsb_items[2];
static qpic_t *rsb_ammo[3];

static qpic_t *hsb_weapons[7][5]; // 0 is active, 1 is owned, 2-5 are flashes
static int hsb_weapon_bits[4] = {HIT_LASER_CANNON_BIT, HIT_MJOLNIR_BIT, 4, HIT_PROXIMITY_GUN_BIT};
static qpic_t *hsb_items[2];

static void Sbar_ShowScores_f (void)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	sb_updated = false;
}

static void Sbar_DontShowScores_f (void)
{
	sb_showscores = false;
	sb_updated = false;
}

void Sbar_Changed (void)
{
	sb_updated = false; // update next frame
}

void Sbar_Init (void)
{
	int i;

	for (i = 0; i < 10; i++)
	{
		sb_nums[0][i] = Draw_PicFromWad (va ("num_%i", i));
		sb_nums[1][i] = Draw_PicFromWad (va ("anum_%i", i));
	}

	sb_nums[0][10] = Draw_PicFromWad ("num_minus");
	sb_nums[1][10] = Draw_PicFromWad ("anum_minus");

	sb_colon = Draw_PicFromWad ("num_colon");
	sb_slash = Draw_PicFromWad ("num_slash");

	sb_weapons[0][0] = Draw_PicFromWad ("inv_shotgun");
	sb_weapons[0][1] = Draw_PicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = Draw_PicFromWad ("inv_nailgun");
	sb_weapons[0][3] = Draw_PicFromWad ("inv_snailgun");
	sb_weapons[0][4] = Draw_PicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = Draw_PicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = Draw_PicFromWad ("inv_lightng");

	sb_weapons[1][0] = Draw_PicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = Draw_PicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = Draw_PicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = Draw_PicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = Draw_PicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = Draw_PicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = Draw_PicFromWad ("inv2_lightng");

	for (i = 0; i < 5; i++)
	{
		sb_weapons[2 + i][0] = Draw_PicFromWad (va ("inva%i_shotgun", i + 1));
		sb_weapons[2 + i][1] = Draw_PicFromWad (va ("inva%i_sshotgun", i + 1));
		sb_weapons[2 + i][2] = Draw_PicFromWad (va ("inva%i_nailgun", i + 1));
		sb_weapons[2 + i][3] = Draw_PicFromWad (va ("inva%i_snailgun", i + 1));
		sb_weapons[2 + i][4] = Draw_PicFromWad (va ("inva%i_rlaunch", i + 1));
		sb_weapons[2 + i][5] = Draw_PicFromWad (va ("inva%i_srlaunch", i + 1));
		sb_weapons[2 + i][6] = Draw_PicFromWad (va ("inva%i_lightng", i + 1));
	}

	sb_ammo[0] = Draw_PicFromWad ("sb_shells");
	sb_ammo[1] = Draw_PicFromWad ("sb_nails");
	sb_ammo[2] = Draw_PicFromWad ("sb_rocket");
	sb_ammo[3] = Draw_PicFromWad ("sb_cells");

	sb_armor[0] = Draw_PicFromWad ("sb_armor1");
	sb_armor[1] = Draw_PicFromWad ("sb_armor2");
	sb_armor[2] = Draw_PicFromWad ("sb_armor3");

	sb_items[0] = Draw_PicFromWad ("sb_key1");
	sb_items[1] = Draw_PicFromWad ("sb_key2");
	sb_items[2] = Draw_PicFromWad ("sb_invis");
	sb_items[3] = Draw_PicFromWad ("sb_invuln");
	sb_items[4] = Draw_PicFromWad ("sb_suit");
	sb_items[5] = Draw_PicFromWad ("sb_quad");

	sb_sigil[0] = Draw_PicFromWad ("sb_sigil1");
	sb_sigil[1] = Draw_PicFromWad ("sb_sigil2");
	sb_sigil[2] = Draw_PicFromWad ("sb_sigil3");
	sb_sigil[3] = Draw_PicFromWad ("sb_sigil4");

	sb_faces[4][0] = Draw_PicFromWad ("face1");
	sb_faces[4][1] = Draw_PicFromWad ("face_p1");
	sb_faces[3][0] = Draw_PicFromWad ("face2");
	sb_faces[3][1] = Draw_PicFromWad ("face_p2");
	sb_faces[2][0] = Draw_PicFromWad ("face3");
	sb_faces[2][1] = Draw_PicFromWad ("face_p3");
	sb_faces[1][0] = Draw_PicFromWad ("face4");
	sb_faces[1][1] = Draw_PicFromWad ("face_p4");
	sb_faces[0][0] = Draw_PicFromWad ("face5");
	sb_faces[0][1] = Draw_PicFromWad ("face_p5");

	sb_face_invis = Draw_PicFromWad ("face_invis");
	sb_face_invuln = Draw_PicFromWad ("face_invul2");
	sb_face_invis_invuln = Draw_PicFromWad ("face_inv2");
	sb_face_quad = Draw_PicFromWad ("face_quad");

	Cmd_AddCommand (src_client, "+showscores", Sbar_ShowScores_f);
	Cmd_AddCommand (src_client, "-showscores", Sbar_DontShowScores_f);

	sb_sbar = Draw_PicFromWad ("sbar");
	sb_ibar = Draw_PicFromWad ("ibar");
	sb_scorebar = Draw_PicFromWad ("scorebar");

	if (hipnotic)
	{
		hsb_weapons[0][0] = Draw_PicFromWad ("inv_laser");
		hsb_weapons[0][1] = Draw_PicFromWad ("inv_mjolnir");
		hsb_weapons[0][2] = Draw_PicFromWad ("inv_gren_prox");
		hsb_weapons[0][3] = Draw_PicFromWad ("inv_prox_gren");
		hsb_weapons[0][4] = Draw_PicFromWad ("inv_prox");

		hsb_weapons[1][0] = Draw_PicFromWad ("inv2_laser");
		hsb_weapons[1][1] = Draw_PicFromWad ("inv2_mjolnir");
		hsb_weapons[1][2] = Draw_PicFromWad ("inv2_gren_prox");
		hsb_weapons[1][3] = Draw_PicFromWad ("inv2_prox_gren");
		hsb_weapons[1][4] = Draw_PicFromWad ("inv2_prox");

		for (i = 0; i < 5; i++)
		{
			hsb_weapons[2 + i][0] = Draw_PicFromWad (va ("inva%i_laser", i + 1));
			hsb_weapons[2 + i][1] = Draw_PicFromWad (va ("inva%i_mjolnir", i + 1));
			hsb_weapons[2 + i][2] = Draw_PicFromWad (va ("inva%i_gren_prox", i + 1));
			hsb_weapons[2 + i][3] = Draw_PicFromWad (va ("inva%i_prox_gren", i + 1));
			hsb_weapons[2 + i][4] = Draw_PicFromWad (va ("inva%i_prox", i + 1));
		}

		hsb_items[0] = Draw_PicFromWad ("sb_wsuit");
		hsb_items[1] = Draw_PicFromWad ("sb_eshld");
	}

	if (rogue)
	{
		rsb_invbar[0] = Draw_PicFromWad ("r_invbar1");
		rsb_invbar[1] = Draw_PicFromWad ("r_invbar2");

		rsb_weapons[0] = Draw_PicFromWad ("r_lava");
		rsb_weapons[1] = Draw_PicFromWad ("r_superlava");
		rsb_weapons[2] = Draw_PicFromWad ("r_gren");
		rsb_weapons[3] = Draw_PicFromWad ("r_multirock");
		rsb_weapons[4] = Draw_PicFromWad ("r_plasma");

		rsb_items[0] = Draw_PicFromWad ("r_shield1");
		rsb_items[1] = Draw_PicFromWad ("r_agrav1");

		rsb_ammo[0] = Draw_PicFromWad ("r_ammolava");
		rsb_ammo[1] = Draw_PicFromWad ("r_ammomulti");
		rsb_ammo[2] = Draw_PicFromWad ("r_ammoplasma");
	}
}

// drawing routines are relative to the status bar location

static void Sbar_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320) >> 1), y + (vid.height - SBAR_HEIGHT), pic);
}

static void Sbar_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320) >> 1), y + (vid.height - SBAR_HEIGHT), pic);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
static void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (x + ((vid.width - 320) >> 1) + 4, y + vid.height - SBAR_HEIGHT, num);
}

static void Sbar_DrawString (int x, int y, char *str)
{
	Draw_String (x + ((vid.width - 320) >> 1), y + vid.height - SBAR_HEIGHT, str);
}

static int Sbar_itoa (int num, char *buf)
{
	char *str;
	int pow10;
	int dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10; num >= pow10; pow10 *= 10);

	do
	{
		pow10 /= 10;
		dig = num / pow10;
		*str++ = '0' + dig;
		num -= dig * pow10;
	} while (pow10 != 1);

	*str = 0;

	return str - buf;
}

static void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char str[12];
	char *ptr;
	int l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l - digits);
	if (l < digits)
		x += (digits - l) * 24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = sb_minus;
		else
			frame = *ptr - '0';

		Sbar_DrawTransPic (x, y, sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

static void Sbar_SoloScoreboard (void)
{
	char str[80];
	int minutes, seconds, tens, units;
	int l;

	sprintf (str, "Monsters:%3i /%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 4, str);

	sprintf (str, "Secrets :%3i /%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (8, 12, str);

	// time
	minutes = cl.time / 60;
	seconds = cl.time - 60 * minutes;
	tens = seconds / 10;
	units = seconds - 10 * tens;
	sprintf (str, "Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

	// draw level name
	l = strlen (cl.levelname);
	Sbar_DrawString (232 - l * 4, 12, cl.levelname);
}

static void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
}

static void Sbar_DrawInventory (void)
{
	int i;
	char num[6];
	float time;
	int flashon;

	if (rogue)
	{
		if (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN)
			Sbar_DrawPic (0, -24, rsb_invbar[0]);
		else
			Sbar_DrawPic (0, -24, rsb_invbar[1]);
	}
	else
		Sbar_DrawPic (0, -24, sb_ibar);

	// weapons
	for (i = 0; i < 7; i++)
	{
		if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << i))
		{
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time) * 10);
			if (flashon < 0)
				flashon = 0;
			if (flashon >= 10)
			{
				if (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i))
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon % 5) + 2;

			Sbar_DrawPic (i * 24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updated = false; // force update to remove flash
		}
	}

	// hipnotic weapons
	if (hipnotic)
	{
		int grenadeflashing = 0;
		for (i = 0; i < 4; i++)
		{
			if (cl.stats[STAT_ITEMS] & (1 << hsb_weapon_bits[i]))
			{
				time = cl.item_gettime[hsb_weapon_bits[i]];
				flashon = (int)((cl.time - time) * 10);
				if (flashon < 0)
					flashon = 0;
				if (flashon >= 10)
				{
					if (cl.stats[STAT_ACTIVEWEAPON] == (1 << hsb_weapon_bits[i]))
						flashon = 1;
					else
						flashon = 0;
				}
				else
					flashon = (flashon % 5) + 2;

				// check grenade launcher
				if (i == 2)
				{
					if (cl.stats[STAT_ITEMS] & HIT_PROXIMITY_GUN)
					{
						if (flashon)
						{
							grenadeflashing = 1;
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
						}
					}
				}
				else if (i == 3)
				{
					if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << 4))
					{
						if (flashon && !grenadeflashing)
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
						else if (!grenadeflashing)
							Sbar_DrawPic (96, -16, hsb_weapons[0][3]);
					}
					else
						Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
				}
				else
					Sbar_DrawPic (176 + (i * 24), -16, hsb_weapons[flashon][i]);
				if (flashon > 1)
					sb_updated = false; // force update to remove flash
			}
		}
	}

	if (rogue)
	{
		// check for powered up weapon.
		if (cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN)
		{
			for (i = 0; i < 5; i++)
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
					Sbar_DrawPic ((i + 2) * 24, -16, rsb_weapons[i]);
		}
	}

	// ammo counts
	for (i = 0; i < 4; i++)
	{
		sprintf (num, "%3i", cl.stats[STAT_SHELLS + i]);
		if (num[0] != ' ')
			Sbar_DrawCharacter ((6 * i + 1) * 8 - 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ((6 * i + 2) * 8 - 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ((6 * i + 3) * 8 - 2, -24, 18 + num[2] - '0');
	}

	flashon = 0;
	// items
	for (i = 0; i < 6; i++)
		if (cl.stats[STAT_ITEMS] & (1 << (17 + i)))
		{
			time = cl.item_gettime[17 + i];
			if (time && time > cl.time - 2 && flashon)
				// flash frame
				sb_updated = false;
			else if (!hipnotic || (i > 1))
				Sbar_DrawPic (192 + i * 16, -16, sb_items[i]);
			if (time && time > cl.time - 2)
				sb_updated = false;
		}
	// hipnotic items
	if (hipnotic)
	{
		for (i = 0; i < 2; i++)
			if (cl.stats[STAT_ITEMS] & (1 << (24 + i)))
			{
				time = cl.item_gettime[24 + i];
				if (time && time > cl.time - 2 && flashon)
					// flash frame
					sb_updated = false;
				else
					Sbar_DrawPic (288 + i * 16, -16, hsb_items[i]);
				if (time && time > cl.time - 2)
					sb_updated = false;
			}
	}

	if (rogue)
	{
		// new rogue items
		for (i = 0; i < 2; i++)
		{
			if (cl.stats[STAT_ITEMS] & (1 << (29 + i)))
			{
				time = cl.item_gettime[29 + i];

				if (time && time > cl.time - 2 && flashon)
					// flash frame
					sb_updated = false;
				else
					Sbar_DrawPic (288 + i * 16, -16, rsb_items[i]);

				if (time && time > cl.time - 2)
					sb_updated = false;
			}
		}
	}
	else
	{
		// sigils
		for (i = 0; i < 4; i++)
		{
			if (cl.stats[STAT_ITEMS] & (1 << (28 + i)))
			{
				time = cl.item_gettime[28 + i];
				if (time && time > cl.time - 2 && flashon)
					// flash frame
					sb_updated = false;
				else
					Sbar_DrawPic (320 - 32 + i * 8, -16, sb_sigil[i]);
				if (time && time > cl.time - 2)
					sb_updated = false;
			}
		}
	}
}

static void Sbar_DrawFace (void)
{
	int f, anim;

	if ((cl.stats[STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY)) == (IT_INVISIBILITY | IT_INVULNERABILITY))
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad);
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis);
		return;
	}
	if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[STAT_HEALTH] / 20;

	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updated = false; // make sure the anim gets drawn over
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

void Sbar_Draw (void)
{
	if (scr_con_current == vid.height)
		return; // console is full screen

	if (sb_updated)
		return;

	sb_updated = true;

	if (sb_lines && (cl_sbar.value != 1 || scr_viewsize.value < 100) && vid.width > 320)
		Draw_TileClear (0, vid.height - sb_lines, vid.width, sb_lines);

	if (sb_lines > 24)
		Sbar_DrawInventory ();

	if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
	{
		Sbar_DrawPic (0, 0, sb_scorebar);
		Sbar_DrawScoreboard ();
		sb_updated = false;
	}
	else if (sb_lines)
	{
		Sbar_DrawPic (0, 0, sb_sbar);

		// keys (hipnotic only)
		if (hipnotic)
		{
			if (cl.stats[STAT_ITEMS] & IT_KEY1)
				Sbar_DrawPic (209, 3, sb_items[0]);
			if (cl.stats[STAT_ITEMS] & IT_KEY2)
				Sbar_DrawPic (209, 12, sb_items[1]);
		}
		// armor
		if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
		{
			Sbar_DrawNum (24, 0, 666, 3, 1);
			Sbar_DrawPic (0, 0, draw_disc);
		}
		else
		{
			if (rogue)
			{
				Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
				if (cl.stats[STAT_ITEMS] & RIT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.stats[STAT_ITEMS] & RIT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.stats[STAT_ITEMS] & RIT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
			else
			{
				Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
				if (cl.stats[STAT_ITEMS] & IT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.stats[STAT_ITEMS] & IT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.stats[STAT_ITEMS] & IT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
		}

		// face
		Sbar_DrawFace ();

		// health
		Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

		// ammo icon
		if (rogue)
		{
			if (cl.stats[STAT_ITEMS] & RIT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.stats[STAT_ITEMS] & RIT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.stats[STAT_ITEMS] & RIT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.stats[STAT_ITEMS] & RIT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
			else if (cl.stats[STAT_ITEMS] & RIT_LAVA_NAILS)
				Sbar_DrawPic (224, 0, rsb_ammo[0]);
			else if (cl.stats[STAT_ITEMS] & RIT_PLASMA_AMMO)
				Sbar_DrawPic (224, 0, rsb_ammo[1]);
			else if (cl.stats[STAT_ITEMS] & RIT_MULTI_ROCKETS)
				Sbar_DrawPic (224, 0, rsb_ammo[2]);
		}
		else
		{
			if (cl.stats[STAT_ITEMS] & IT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.stats[STAT_ITEMS] & IT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.stats[STAT_ITEMS] & IT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.stats[STAT_ITEMS] & IT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
		}

		Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);
	}
}

static void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char str[12];
	char *ptr;
	int l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l - digits);
	if (l < digits)
		x += (digits - l) * 24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = sb_minus;
		else
			frame = *ptr - '0';

		Draw_TransPic (x, y, sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

void Sbar_IntermissionOverlay (void)
{
	qpic_t *pic;
	int dig;
	int num;

	pic = Draw_CachePic ("gfx/complete.lmp");
	Draw_Pic (64, 24, pic);

	pic = Draw_CachePic ("gfx/inter.lmp");
	Draw_TransPic (0, 56, pic);

	// time
	dig = cl.completed_time / 60;
	Sbar_IntermissionNumber (160, 64, dig, 3, 0);
	num = cl.completed_time - dig * 60;
	Draw_TransPic (234, 64, sb_colon);
	Draw_TransPic (246, 64, sb_nums[0][num / 10]);
	Draw_TransPic (266, 64, sb_nums[0][num % 10]);

	Sbar_IntermissionNumber (160, 104, cl.stats[STAT_SECRETS], 3, 0);
	Draw_TransPic (232, 104, sb_slash);
	Sbar_IntermissionNumber (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);

	Sbar_IntermissionNumber (160, 144, cl.stats[STAT_MONSTERS], 3, 0);
	Draw_TransPic (232, 144, sb_slash);
	Sbar_IntermissionNumber (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);
}

void Sbar_FinaleOverlay (void)
{
	qpic_t *pic;

	pic = Draw_CachePic ("gfx/finale.lmp");
	Draw_TransPic ((vid.width - pic->width) / 2, 16, pic);
}
