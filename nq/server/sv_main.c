/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2023 Justin Keller

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

server_t		sv;
server_static_t	svs;

char	localmodels[MAX_MODELS][5];			// inline model names for precache

//============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init ()
{
	int		i;
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_nostep;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_edgefriction;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_maxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_idealpitchscale;
	extern	cvar_t	sv_aim;

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_edgefriction);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_idealpitchscale);
	Cvar_RegisterVariable (&sv_aim);
	Cvar_RegisterVariable (&sv_nostep);

	for (i=0 ; i<MAX_MODELS ; i++)
		sprintf (localmodels[i], "*%i", i);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*  
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;	
	MSG_WriteByte (&sv.datagram, svc_particle);
	MSG_WriteCoord (&sv.datagram, org[0]);
	MSG_WriteCoord (&sv.datagram, org[1]);
	MSG_WriteCoord (&sv.datagram, org[2]);
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar (&sv.datagram, v);
	}
	MSG_WriteByte (&sv.datagram, count);
	MSG_WriteByte (&sv.datagram, color);
}           

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation)
{       
    int         sound_num;
    int field_mask;
    int			i;
	int			ent;
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	if (sv.datagram.cursize > MAX_DATAGRAM-16)
		return;	

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS
        && sv.sound_precache[sound_num] ; sound_num++)
        if (!strcmp(sample, sv.sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS || !sv.sound_precache[sound_num] )
    {
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

// directed messages go only to the entity the are targeted on
	MSG_WriteByte (&sv.datagram, svc_sound);
	MSG_WriteByte (&sv.datagram, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&sv.datagram, volume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&sv.datagram, attenuation*64);
	MSG_WriteShort (&sv.datagram, channel);
	MSG_WriteByte (&sv.datagram, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&sv.datagram, ed_vector(entity, origin)[i]+0.5*(ed_vector(entity, mins)[i]+ed_vector(entity, maxs)[i]));
}           

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	char			**s;
	char			message[2048];

	MSG_WriteByte (&client->message, svc_print);
	sprintf (message, "%c\nVERSION "QUAKE_VERSION" SERVER (%i CRC)\n", 2, sv.pr.crc);
	MSG_WriteString (&client->message,message);

	MSG_WriteByte (&client->message, svc_serverinfo);
	MSG_WriteLong (&client->message, PROTOCOL_VERSION);
	MSG_WriteByte (&client->message, svs.maxclients);

	if (!coop.value && deathmatch.value)
		MSG_WriteByte (&client->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->message, GAME_COOP);

	strcpy (message, ed_get_string(sv.edicts, message));

	MSG_WriteString (&client->message,message);

	for (s = sv.model_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

	for (s = sv.sound_precache+1 ; *s ; s++)
		MSG_WriteString (&client->message, *s);
	MSG_WriteByte (&client->message, 0);

// send music
	MSG_WriteByte (&client->message, svc_cdtrack);
	MSG_WriteByte (&client->message, ed_float(sv.edicts, sounds));
	MSG_WriteByte (&client->message, ed_float(sv.edicts, sounds));

// set view	
	MSG_WriteByte (&client->message, svc_setview);
	MSG_WriteShort (&client->message, NUM_FOR_EDICT(client->edict));

	MSG_WriteByte (&client->message, svc_signonnum);
	MSG_WriteByte (&client->message, 1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum)
{
	edict_t			*ent;
	client_t		*client;
	int				edictnum;
	struct qsocket_s *netconnection;
	int				i;
	float			spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients + clientnum;

	Con_DPrintf ("Client %s connected\n", client->netconnection->address);

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);
	
// set up the client_t
	netconnection = client->netconnection;
	
	if (sv.loadgame)
		memcpy (spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	memset (client, 0, sizeof(*client));
	client->netconnection = netconnection;

	strcpy (client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;
	client->message.data = client->msgbuf;
	client->message.maxsize = sizeof(client->msgbuf);
	client->message.allowoverflow = true;		// we can catch it
	client->privileged = false;

	if (sv.loadgame)
		memcpy (client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	else
	{
	// call the progs to get default spawn parms for the new client
		sv_pr_execute(SetNewParms);
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = sv_pr_vector(parm1)[i];
	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients ()
{
	struct qsocket_s	*ret;
	int				i;
		
//
// check for new connections
//
	while (1)
	{
		ret = NET_CheckNewConnections ();
		if (!ret)
			break;

	// 
	// init a new client structure
	//	
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("Host_CheckForNewClients: no free clients");
		
		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);	
	
		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram ()
{
	SZ_Clear (&sv.datagram);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS/8];

void SV_AddToFatPVS (vec3_t org, mnode_t *node)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = CMod_LeafPVS ( (mleaf_t *)node, sv.worldmodel);
				for (i=0 ; i<fatbytes ; i++)
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
		{	// go down both
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
byte *SV_FatPVS (vec3_t org)
{
	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	memset (fatpvs, 0, fatbytes);
	SV_AddToFatPVS (org, sv.worldmodel->nodes);
	return fatpvs;
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

=============
*/
void SV_WriteEntitiesToClient (edict_t	*clent, sizebuf_t *msg)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	edict_t	*ent;

// find the client's PVS
	VectorAdd (ed_vector(clent, origin), ed_vector(clent, view_ofs), org);
	pvs = SV_FatPVS (org);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ed_float(ent, effects) == EF_NODRAW)
			continue;

// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
// ignore ents without visible models
			if (!ed_float(ent, modelindex) || ed_get_string(ent, model)[0] == '\0')
				continue;

			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;
				
			if (i == ent->num_leafs)
				continue;		// not visible
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			Con_Printf ("packet overflow\n");
			return;
		}

// send an update
		bits = 0;
		
		for (i=0 ; i<3 ; i++)
		{
			miss = ed_vector(ent, origin)[i] - ent->baseline.origin[i];
			if ( miss < -0.1 || miss > 0.1 )
				bits |= U_ORIGIN1<<i;
		}

		if ( ed_vector(ent, angles)[0] != ent->baseline.angles[0] )
			bits |= U_ANGLE1;
			
		if ( ed_vector(ent, angles)[1] != ent->baseline.angles[1] )
			bits |= U_ANGLE2;
			
		if ( ed_vector(ent, angles)[2] != ent->baseline.angles[2] )
			bits |= U_ANGLE3;
			
		if (ed_float(ent, movetype) == MOVETYPE_STEP)
			bits |= U_NOLERP;	// don't mess up the step animation
	
		if (ent->baseline.colormap != ed_float(ent, colormap))
			bits |= U_COLORMAP;
			
		if (ent->baseline.skin != ed_float(ent, skin))
			bits |= U_SKIN;
			
		if (ent->baseline.frame != ed_float(ent, frame))
			bits |= U_FRAME;
		
		if (ent->baseline.effects != ed_float(ent, effects))
			bits |= U_EFFECTS;
		
		if (ent->baseline.modelindex != ed_float(ent, modelindex))
			bits |= U_MODEL;

		if (e >= 256)
			bits |= U_LONGENTITY;
			
		if (bits >= 256)
			bits |= U_MOREBITS;

	//
	// write the message
	//
		MSG_WriteByte (msg,bits | U_SIGNAL);
		
		if (bits & U_MOREBITS)
			MSG_WriteByte (msg, bits>>8);
		if (bits & U_LONGENTITY)
			MSG_WriteShort (msg,e);
		else
			MSG_WriteByte (msg,e);

		if (bits & U_MODEL)
			MSG_WriteByte (msg,	ed_float(ent, modelindex));
		if (bits & U_FRAME)
			MSG_WriteByte (msg, ed_float(ent, frame));
		if (bits & U_COLORMAP)
			MSG_WriteByte (msg, ed_float(ent, colormap));
		if (bits & U_SKIN)
			MSG_WriteByte (msg, ed_float(ent, skin));
		if (bits & U_EFFECTS)
			MSG_WriteByte (msg, ed_float(ent, effects));
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (msg, ed_vector(ent, origin)[0]);		
		if (bits & U_ANGLE1)
			MSG_WriteAngle(msg, ed_vector(ent, angles)[0]);
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (msg, ed_vector(ent, origin)[1]);
		if (bits & U_ANGLE2)
			MSG_WriteAngle(msg, ed_vector(ent, angles)[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (msg, ed_vector(ent, origin)[2]);
		if (bits & U_ANGLE3)
			MSG_WriteAngle(msg, ed_vector(ent, angles)[2]);
	}
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts ()
{
	int		e;
	edict_t	*ent;
	
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		ed_float(ent, effects) = (int)ed_float(ent, effects) & ~EF_MUZZLEFLASH;
	}

}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg)
{
	int		bits;
	int		i;
	edict_t	*other;
	int		items;

//
// send a damage message
//
	if (ed_float(ent, dmg_take) || ed_float(ent, dmg_save))
	{
		other = ed_get_edict(ent, dmg_inflictor);
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, ed_float(ent, dmg_save));
		MSG_WriteByte (msg, ed_float(ent, dmg_take));
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, ed_vector(other, origin)[i] + 0.5*(ed_vector(other, mins)[i] + ed_vector(other, maxs)[i]));
	
		ed_float(ent, dmg_take) = 0;
		ed_float(ent, dmg_save) = 0;
	}

//
// send the current viewpos offset from the view entity
//
	if (ed_field(idealpitch))
	{
		SV_SetIdealPitch ();		// how much to look up / down ideally
	}

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ed_float(ent, fixangle) )
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ed_vector(ent, angles)[i] );
		ed_float(ent, fixangle) = 0;
	}

	bits = 0;
	
	if (ed_vector(ent, view_ofs)[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;
		
	if (ed_field(idealpitch) && ed_float(ent, idealpitch))
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2

	if (ed_field(items2))
	{
		items = (int)ed_float(ent, items) | ((int)ed_float(ent, items2) << 23);
	}
	else
	{
		items = (int)ed_float(ent, items) | ((int)sv_pr_float(serverflags) << 28);
	}

	bits |= SU_ITEMS;
	
	if ( (int)ed_float(ent, flags) & FL_ONGROUND)
		bits |= SU_ONGROUND;
	
	if ( ed_float(ent, waterlevel) >= 2)
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
		if (ed_field(punchangle) && ed_vector(ent, punchangle)[i])
			bits |= (SU_PUNCH1<<i);
		if (ed_vector(ent, velocity)[i])
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (ed_float(ent, weaponframe))
		bits |= SU_WEAPONFRAME;

	if (ed_float(ent, armorvalue))
		bits |= SU_ARMOR;

	bits |= SU_WEAPON;

// send the data

	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ed_vector(ent, view_ofs)[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ed_float(ent, idealpitch));

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			MSG_WriteChar (msg, ed_vector(ent, punchangle)[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ed_vector(ent, velocity)[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ed_float(ent, weaponframe));
	if (bits & SU_ARMOR)
		MSG_WriteByte (msg, ed_float(ent, armorvalue));
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(ed_get_string(ent, weaponmodel)));
	
	MSG_WriteShort (msg, ed_float(ent, health));
	MSG_WriteByte (msg, ed_float(ent, currentammo));
	MSG_WriteByte (msg, ed_float(ent, ammo_shells));
	MSG_WriteByte (msg, ed_float(ent, ammo_nails));
	MSG_WriteByte (msg, ed_float(ent, ammo_rockets));
	MSG_WriteByte (msg, ed_float(ent, ammo_cells));

	if (standard_quake)
	{
		MSG_WriteByte (msg, ed_float(ent, weapon));
	}
	else
	{
		for(i=0;i<32;i++)
		{
			if ( ((int)ed_float(ent, weapon)) & (1<<i) )
			{
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
bool SV_SendClientDatagram (client_t *client)
{
	byte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteByte (&msg, svc_time);
	MSG_WriteFloat (&msg, sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client->edict, &msg);

	SV_WriteEntitiesToClient (client->edict, &msg);

// copy the server datagram if there is space
	if (msg.cursize + sv.datagram.cursize < msg.maxsize)
		SZ_Write (&msg, sv.datagram.data, sv.datagram.cursize);

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}
	
	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages ()
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->old_frags != ed_float(host_client->edict, frags))
		{
			for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;
				MSG_WriteByte (&client->message, svc_updatefrags);
				MSG_WriteByte (&client->message, i);
				MSG_WriteShort (&client->message, ed_float(host_client->edict, frags));
			}

			host_client->old_frags = ed_float(host_client->edict, frags);
		}
	}
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		SZ_Write (&client->message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
	}

	SZ_Clear (&sv.reliable_datagram);
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop (client_t *client)
{
	sizebuf_t	msg;
	byte		buf[4];
	
	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages ()
{
	int			i;
	
// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate 
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}
			
		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
				SV_DropClient (false);	// went to another level
			else
			{
				if (NET_SendMessage (host_client->netconnection
				, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				SZ_Clear (&host_client->message);
				host_client->last_message = realtime;
				host_client->sendsignon = false;
			}
		}
	}
	
	
// clear muzzle flashes
	SV_CleanupEnts ();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (char *name)
{
	int		i;
	
	if (!name || !name[0])
		return 0;

	for (i=0 ; i<MAX_MODELS && sv.model_precache[i] ; i++)
		if (!strcmp(sv.model_precache[i], name))
			return i;
	if (i==MAX_MODELS || !sv.model_precache[i])
		Sys_Error ("SV_ModelIndex: model %s not precached", name);
	return i;
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline ()
{
	int			i;
	edict_t			*svent;
	int				entnum;	
		
	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !ed_float(svent, modelindex))
			continue;

	//
	// create entity baseline
	//
		VectorCopy (ed_vector(svent, origin), svent->baseline.origin);
		VectorCopy (ed_vector(svent, angles), svent->baseline.angles);
		svent->baseline.frame = ed_float(svent, frame);
		svent->baseline.skin = ed_float(svent, skin);
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(ed_get_string(svent, model));
		}
		
	//
	// add to the message
	//
		MSG_WriteByte (&sv.signon,svc_spawnbaseline);		
		MSG_WriteShort (&sv.signon,entnum);

		MSG_WriteByte (&sv.signon, svent->baseline.modelindex);
		MSG_WriteByte (&sv.signon, svent->baseline.frame);
		MSG_WriteByte (&sv.signon, svent->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->baseline.skin);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
			MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect ()
{
	char	data[128];
	sizebuf_t	msg;

	msg.data = data;
	msg.cursize = 0;
	msg.maxsize = sizeof(data);

	MSG_WriteChar (&msg, svc_stufftext);
	MSG_WriteString (&msg, "reconnect\n");
	NET_SendToAll (&msg, 5);

	if (Host_IsLocalClient ())
		Cmd_ExecuteString ("reconnect\n", src_command);
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms ()
{
	int		i, j;

	svs.serverflags = sv_pr_float(serverflags);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		sv_pr_int(self) = EDICT_TO_PROG(host_client->edict);
		sv_pr_execute(SetChangeParms);
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = sv_pr_vector(parm1)[j];
	}
}


static bool SV_LoadProgs()
{
	if (PR_LoadProgs(&sv.pr, "progs.dat", PROG_VERSION, PROGHEADER_CRC) != 0)
	{
		return false;
	}

	#define PR_FIELD(_, name) {#name, false},
	#define PR_FIELD_OPTIONAL(_, name) {#name, true},

	pr_field_t pr_globals[] =
	{
		#include "pr_globals.h"
		{NULL, true}
	};

	pr_field_t pr_fields[] =
	{
		#include "pr_fields.h"
		{NULL, true}
	};

	#undef PR_FIELD
	#undef PR_FIELD_OPTIONAL

	uint32_t *pr_global_struct = Hunk_AllocName((pr_globals_count + pr_fields_count) * sizeof(uint32_t), "pr_tables");
	uint32_t *pr_fields_struct = pr_global_struct + pr_globals_count;

	PR_BuildStructs(&sv.pr, pr_global_struct, pr_globals, pr_fields_struct, pr_fields);

	sv.pr.builtins = pr_builtins;
	sv.pr.numbuiltins = pr_numbuiltins;

	return true;
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/

void SV_SpawnServer (char *server, char *startspot)
{
	edict_t		*ent;
	int			i;

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
	{
		Cvar_Set ("hostname", "UNNAMED");
	}

	Con_DPrintf ("SpawnServer: %s\n",server);
	svs.changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (Host_IsLocalGame ())
	{
		SV_SendReconnect ();
	}

//
// make cvars consistant
//
	if (coop.value)
		Cvar_SetValue ("deathmatch", 0);
	current_skill = (int)(skill.value + 0.5);
	if (current_skill < 0)
		current_skill = 0;
	if (current_skill > 3)
		current_skill = 3;

	Cvar_SetValue ("skill", (float)current_skill);
	
//
// set up the new server
//
	Host_ClearMemory ();

	memset (&sv, 0, sizeof(sv));

	strcpy (sv.name, server);
	if (startspot)
		strcpy(sv.startspot, startspot);

// load progs to get entity field count
	if (!SV_LoadProgs())
	{
		sv.active = false;
		return;
	}

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;
	sv.datagram.allowoverflow = true;
	
	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;
	
	sv.signon.maxsize = sizeof(sv.signon_buf);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buf;
	
	sv.state = ss_loading;
	sv.paused = false;

	sv.time = 1.0;
	
	strcpy (sv.name, server);
	sprintf (sv.modelname,"maps/%s.bsp", server);
	sv.worldmodel = CMod_ForName (sv.modelname, false, true);
	if (!sv.worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", sv.modelname);
		sv.active = false;
		return;
	}
	sv.models[1] = sv.worldmodel;

	size_t numEntities = sv.worldmodel->numentities;
	// Throw in an extra 20% for good measure.
	// Round to the next highest multiple of 8, cause why not.
	sv.max_edicts = (size_t)ceil(numEntities * 1.2 / 8.0) * 8;

	if (sv.max_edicts < MIN_EDICTS)
	{
		sv.max_edicts = MIN_EDICTS;
	}

	Con_DPrintf("SV_SpawnServer: Allocating %lu edicts (Counted %lu)\n", sv.max_edicts, numEntities);

	// Allocate server memory.
	sv.edicts = Hunk_AllocName(sv.max_edicts * sv.pr.edict_size, "edicts");
	
	// Leave slots at start for clients only.
	sv.num_edicts = svs.maxclients + 1;
	for (i = 0; i < svs.maxclients; i++)
	{
		ent = EDICT_NUM(i + 1);
		svs.clients[i].edict = ent;
	}

//
// clear world interaction links
//
	SV_ClearWorld ();
	
	sv.sound_precache[0] = sv.pr.strings;

	sv.model_precache[0] = sv.pr.strings;
	sv.model_precache[1] = sv.modelname;
	for (i=1 ; i<sv.worldmodel->numsubmodels ; i++)
	{
		sv.model_precache[1+i] = localmodels[i];
		sv.models[i+1] = CMod_ForName (localmodels[i], false, false);
	}

//
// load the rest of the entities
//	
	ent = EDICT_NUM(0);
	ED_ClearEdict (ent);
	ed_set_string(ent, model, sv.worldmodel->name);
	ed_float(ent, modelindex) = 1;		// world model
	ed_float(ent, solid) = SOLID_BSP;
	ed_float(ent, movetype) = MOVETYPE_PUSH;

	if (coop.value)
	{
		if (pr_field(coop))
		{
			sv_pr_float(coop) = coop.value;
		}
	}
	else
	{
		sv_pr_float(deathmatch) = deathmatch.value;
	}

	sv_pr_int(mapname) = PR_SetString(&sv.pr, sv.name);
#ifdef QUAKE2
	sv_pr_int(startspot) = PR_SetString(&sv.pr, sv.startspot);
#endif

// serverflags are for cross level information (sigils)
	sv_pr_float(serverflags) = svs.serverflags;
	
	ED_LoadFromFile (sv.worldmodel->entities);

	sv.active = true;

// all setup is completed, any further precache statements are errors
	sv.state = ss_active;
	
// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// create a baseline for more efficient communications
	SV_CreateBaseline ();

// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_SendServerinfo (host_client);
	
	Con_DPrintf ("Server spawned.\n");
}

