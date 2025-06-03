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

#include "serverdef.h"

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static int fatbytes;
static byte fatpvs[MAX_MAP_LEAFS / 8];

static void SV_AddToFatPVS (vec3_t org, mnode_t *node)
{
	int i;
	byte *pvs;
	mplane_t *plane;
	float d;

	while (1)
	{
		// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = CMod_LeafPVS ((mleaf_t *)node, sv.worldmodel);
				for (i = 0; i < fatbytes; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}

		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{ // go down both
			SV_AddToFatPVS (org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
static byte *SV_FatPVS (vec3_t org)
{
	fatbytes = (sv.worldmodel->numleafs + 31) >> 3;
	memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, sv.worldmodel->nodes);
	return fatpvs;
}

//=============================================================================

// because there can be a lot of nails, there is a special
// network protocol for them

#define MAX_NAILS 32

static edict_t *nails[MAX_NAILS];
static int numnails;

extern int sv_nailmodel, sv_supernailmodel, sv_playermodel;

static bool SV_AddNailUpdate (edict_t *ent)
{
	if (ed_float (ent, modelindex) != sv_nailmodel && ed_float (ent, modelindex) != sv_supernailmodel)
	{
		return false;
	}

	if (numnails == MAX_NAILS)
	{
		return true;
	}

	nails[numnails] = ent;
	numnails++;

	return true;
}

static void SV_EmitNailUpdate (sizebuf_t *msg)
{
	byte bits[6]; // [48 bits] xyzpy 12 12 12 4 8
	int n, i;
	edict_t *ent;
	int x, y, z, p, yaw;

	if (!numnails)
		return;

	MSG_WriteByte (msg, svc_nails);
	MSG_WriteByte (msg, numnails);

	for (n = 0; n < numnails; n++)
	{
		ent = nails[n];
		float *entOrigin = ed_vector (ent, origin);
		float *entAngles = ed_vector (ent, angles);
		x = (int)(entOrigin[0] + 4096) >> 1;
		y = (int)(entOrigin[1] + 4096) >> 1;
		z = (int)(entOrigin[2] + 4096) >> 1;
		p = (int)(16 * entAngles[PITCH] / 360) & 15;
		yaw = (int)(256 * entAngles[YAW] / 360) & 255;

		bits[0] = x;
		bits[1] = (x >> 8) | (y << 4);
		bits[2] = (y >> 4);
		bits[3] = z;
		bits[4] = (z >> 8) | (p << 4);
		bits[5] = yaw;

		for (i = 0; i < 6; i++)
			MSG_WriteByte (msg, bits[i]);
	}
}

//=============================================================================

/*
==================
SV_WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
static void SV_WriteDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, bool force)
{
	int bits;
	int i;
	float miss;

	// send an update
	bits = 0;

	for (i = 0; i < 3; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if (miss < -0.1 || miss > 0.1)
			bits |= U_ORIGIN1 << i;
	}

	if (to->angles[0] != from->angles[0])
		bits |= U_ANGLE1;

	if (to->angles[1] != from->angles[1])
		bits |= U_ANGLE2;

	if (to->angles[2] != from->angles[2])
		bits |= U_ANGLE3;

	if (to->colormap != from->colormap)
		bits |= U_COLORMAP;

	if (to->skinnum != from->skinnum)
		bits |= U_SKIN;

	if (to->frame != from->frame)
		bits |= U_FRAME;

	if (to->effects != from->effects)
		bits |= U_EFFECTS;

	if (to->modelindex != from->modelindex)
		bits |= U_MODEL;

	if (bits & 511)
		bits |= U_MOREBITS;

	if (to->flags & U_SOLID)
		bits |= U_SOLID;

	//
	// write the message
	//
	if (!to->number)
		Host_Error ("Unset entity number");
	if (to->number >= 512)
		Host_Error ("Entity number >= 512");

	if (!bits && !force)
		return; // nothing to send!
	i = to->number | (bits & ~511);
	if (i & U_REMOVE)
		Sys_Error ("U_REMOVE");
	MSG_WriteShort (msg, i);

	if (bits & U_MOREBITS)
		MSG_WriteByte (msg, bits & 255);
	if (bits & U_MODEL)
		MSG_WriteByte (msg, to->modelindex);
	if (bits & U_FRAME)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		MSG_WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		MSG_WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		MSG_WriteByte (msg, to->effects);
	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);
	if (bits & U_ANGLE1)
		MSG_WriteAngle (msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle (msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle (msg, to->angles[2]);
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of a packet_entities_t to the message.
=============
*/
static void SV_EmitPacketEntities (client_t *client, packet_entities_t *to, sizebuf_t *msg)
{
	edict_t *ent;
	client_frame_t *fromframe;
	packet_entities_t *from;
	int oldindex, newindex;
	int oldnum, newnum;
	int oldmax;

	// this is the frame that we are going to delta update from
	if (client->delta_sequence != -1)
	{
		fromframe = &client->frames[client->delta_sequence & UPDATE_MASK];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		MSG_WriteByte (msg, svc_deltapacketentities);
		MSG_WriteByte (msg, client->delta_sequence);
	}
	else
	{
		oldmax = 0; // no delta update
		from = NULL;

		MSG_WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
	//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
	//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{	// delta update from old position
			//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (&from->entities[oldindex], &to->entities[newindex], msg, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{ // this is a new entity, send it from the baseline
			ent = EDICT_NUM (newnum);
			//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (&ent->baseline, &to->entities[newindex], msg, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
			//Con_Printf ("remove %i\n", oldnum);
			MSG_WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0); // end of packetentities
}

/*
=============
SV_WritePlayersToClient
=============
*/
static void SV_WritePlayersToClient (client_t *client, edict_t *clent, byte *pvs, sizebuf_t *msg)
{
	int i, j;
	client_t *cl;
	edict_t *ent;
	int msec;
	usercmd_t cmd;
	int pflags;

	for (j = 0, cl = svs.clients; j < MAX_CLIENTS; j++, cl++)
	{
		if (cl->state != cs_spawned)
			continue;

		ent = cl->edict;

		// ZOID visibility tracking
		if (ent != clent && !(client->spec_track && client->spec_track - 1 == j))
		{
			if (cl->spectator)
				continue;

			// ignore if not touching a PV leaf
			for (i = 0; i < ent->num_leafs; i++)
			{
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7)))
				{
					break;
				}
			}

			if (i == ent->num_leafs)
				continue; // not visible
		}

		pflags = PF_MSEC | PF_COMMAND;

		if (ed_float (ent, modelindex) != sv_playermodel)
			pflags |= PF_MODEL;
		for (i = 0; i < 3; i++)
		{
			if (ed_vector (ent, velocity)[i])
			{
				pflags |= PF_VELOCITY1 << i;
			}
		}
		if (ed_float (ent, effects))
			pflags |= PF_EFFECTS;
		if (ed_float (ent, skin))
			pflags |= PF_SKINNUM;
		if (ed_float (ent, health) <= 0)
			pflags |= PF_DEAD;
		if (ed_vector (ent, mins)[2] != -24)
			pflags |= PF_GIB;

		if (cl->spectator)
		{ // only sent origin and velocity to spectators
			pflags &= PF_VELOCITY1 | PF_VELOCITY2 | PF_VELOCITY3;
		}
		else if (ent == clent)
		{ // don't send a lot of data on personal entity
			pflags &= ~(PF_MSEC | PF_COMMAND);
			if (ed_float (ent, weaponframe))
				pflags |= PF_WEAPONFRAME;
		}

		if (client->spec_track && client->spec_track - 1 == j && ed_float (ent, weaponframe))
		{
			pflags |= PF_WEAPONFRAME;
		}

		MSG_WriteByte (msg, svc_playerinfo);
		MSG_WriteByte (msg, j);
		MSG_WriteShort (msg, pflags);

		for (i = 0; i < 3; i++)
		{
			MSG_WriteCoord (msg, ed_vector (ent, origin)[i]);
		}

		MSG_WriteByte (msg, ed_float (ent, frame));

		if (pflags & PF_MSEC)
		{
			msec = 1000 * (sv.time - cl->localtime);
			if (msec > 255)
				msec = 255;
			MSG_WriteByte (msg, msec);
		}

		if (pflags & PF_COMMAND)
		{
			cmd = cl->lastcmd;

			// don't show the corpse looking around...
			if (ed_float (ent, health) <= 0)
			{
				cmd.angles[PITCH] = 0;
				cmd.angles[YAW] = ed_vector (ent, angles)[YAW];
				cmd.angles[ROLL] = 0;
			}

			cmd.buttons = 0; // never send buttons
			cmd.impulse = 0; // never send impulses

			MSG_WriteDeltaUsercmd (msg, &nullcmd, &cmd);
		}

		for (i = 0; i < 3; i++)
		{
			if (pflags & (PF_VELOCITY1 << i))
			{
				MSG_WriteShort (msg, ed_vector (ent, velocity)[i]);
			}
		}

		if (pflags & PF_MODEL)
			MSG_WriteByte (msg, ed_float (ent, modelindex));

		if (pflags & PF_SKINNUM)
			MSG_WriteByte (msg, ed_float (ent, skin));

		if (pflags & PF_EFFECTS)
			MSG_WriteByte (msg, ed_float (ent, effects));

		if (pflags & PF_WEAPONFRAME)
			MSG_WriteByte (msg, ed_float (ent, weaponframe));
	}
}

/*
=============
SV_SendCompatibilityMessages
=============
*/
void SV_SendCompatibilityMessages ()
{
	int e;
	edict_t *ent;
	float *punchangle;
	client_t *cl;

	for (e = 0, cl = svs.clients; e < MAX_CLIENTS; e++, cl++)
	{
		if (cl->state != cs_spawned)
		{
			continue;
		}

		ent = cl->edict;
		punchangle = ed_vector (ent, punchangle);

		/* Send the player view punch to the client. */
		if (ed_field (punchangle))
		{
			if (punchangle[PITCH] <= -4.0f)
			{
				ClientReliableWrite_Begin (cl, svc_bigkick, 1);
			}
			else if (punchangle[PITCH] <= -2.0f)
			{
				ClientReliableWrite_Begin (cl, svc_smallkick, 1);
			}

			punchangle[PITCH] = punchangle[YAW] = punchangle[ROLL] = 0.0f;
		}

		/* Send player muzzle flashes & unset the bit. */
		if ((int)ed_float (ent, effects) & EF_MUZZLEFLASH)
		{
			MSG_WriteByte (&sv.multicast, svc_muzzleflash);
			MSG_WriteShort (&sv.multicast, NUM_FOR_EDICT (ent));

			SV_Multicast (ed_vector (ent, origin), MULTICAST_PVS);

			ed_float (ent, effects) = (int)ed_float (ent, effects) & ~EF_MUZZLEFLASH;
		}
	}
}

/*
=============
SV_CleanupEnts
=============
*/
void SV_CleanupEnts ()
{
	int e;
	edict_t *ent;

	/* Clear non-player muzzle flashes. */
	for (e = MAX_CLIENTS + 1, ent = EDICT_NUM (e); e < sv.num_edicts; e++, ent = NEXT_EDICT (ent))
	{
		ed_float (ent, effects) = (int)ed_float (ent, effects) & ~EF_MUZZLEFLASH;
	}
}

/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a svc_packetentities messages and possibly
a svc_nails message and
svc_playerinfo messages
=============
*/
void SV_WriteEntitiesToClient (client_t *client, sizebuf_t *msg)
{
	int e, i;
	byte *pvs;
	vec3_t org;
	edict_t *ent;
	packet_entities_t *pack;
	edict_t *clent;
	client_frame_t *frame;
	entity_state_t *state;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.incoming_sequence & UPDATE_MASK];

	// find the client's PVS
	clent = client->edict;
	VectorAdd (ed_vector (clent, origin), ed_vector (clent, view_ofs), org);
	pvs = SV_FatPVS (org);

	// send over the players in the PVS
	SV_WritePlayersToClient (client, clent, pvs, msg);

	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;

	numnails = 0;

	for (e = MAX_CLIENTS + 1, ent = EDICT_NUM (e); e < sv.num_edicts; e++, ent = NEXT_EDICT (ent))
	{
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ed_float (ent, effects) == EF_NODRAW)
			continue;

		// ignore ents without visible models
		if (!ed_float (ent, modelindex) || ed_get_string (ent, model)[0] == '\0')
			continue;

		// ignore if not touching a PV leaf
		for (i = 0; i < ent->num_leafs; i++)
		{
			if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7)))
			{
				break;
			}
		}

		if (i == ent->num_leafs)
			continue; // not visible

		if (SV_AddNailUpdate (ent))
			continue; // added to the special update list

		// add to the packetentities
		if (pack->num_entities == MAX_PACKET_ENTITIES)
			continue; // all full

		state = &pack->entities[pack->num_entities];
		pack->num_entities++;

		state->number = e;
		state->flags = 0;
		VectorCopy (ed_vector (ent, origin), state->origin);
		VectorCopy (ed_vector (ent, angles), state->angles);
		state->modelindex = ed_float (ent, modelindex);
		state->frame = ed_float (ent, frame);
		state->colormap = ed_float (ent, colormap);
		state->skinnum = ed_float (ent, skin);
		state->effects = ed_float (ent, effects);
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities (client, pack, msg);

	// now add the specialized nail update
	SV_EmitNailUpdate (msg);
}
