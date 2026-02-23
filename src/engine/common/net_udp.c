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

#include "bothdef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

netadr_t net_from;
sizebuf_t net_message[SOCKETS];
int net_socket[SOCKETS];

static netadr_t net_local_adr;
static char net_public_adr[24];

#define MAX_UDP_PACKET 8192
static byte net_message_buffer[SOCKETS][MAX_UDP_PACKET];

static byte net_loopback_buffer[SOCKETS][MAX_UDP_PACKET];
static size_t net_loopback_size[SOCKETS];

static void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
	memset (s, 0, sizeof (*s));
	s->sin_family = AF_INET;

	*(int32_t *)&s->sin_addr = *(int32_t *)&a->ip;
	s->sin_port = htons (a->port);
}

static void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a)
{
	*(int32_t *)&a->ip = *(int32_t *)&s->sin_addr;
	a->port = ntohs (s->sin_port);
}

bool NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
		return true;
	return false;
}

bool NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

char *NET_AdrToString (netadr_t a)
{
	static char s[64];

	sprintf (s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs (a.port));

	return s;
}

char *NET_BaseAdrToString (netadr_t a)
{
	static char s[64];

	sprintf (s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool NET_StringToAdr (char *s, netadr_t *a)
{
	struct hostent *h;
	struct sockaddr_in sadr;
	char *colon;
	char copy[128];

	if (strcmp (s, "localhost") == 0)
	{
		a->ip[0] = 127;
		a->ip[1] = 0;
		a->ip[2] = 0;
		a->ip[3] = 1;
		a->port = 0;
		return true;
	}

	memset (&sadr, 0, sizeof (sadr));
	sadr.sin_family = AF_INET;

	sadr.sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy; *colon; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			sadr.sin_port = htons (atoi (colon + 1));
		}

	if (copy[0] >= '0' && copy[0] <= '9')
		*(int32_t *)&sadr.sin_addr = inet_addr (copy);
	else
	{
		if (!(h = gethostbyname (copy)))
			return false;
		*(int32_t *)&sadr.sin_addr = *(int32_t *)h->h_addr_list[0];
	}

	SockadrToNetadr (&sadr, a);

	return true;
}

bool NET_IsLocalHost (netadr_t *adr)
{
	return adr->ip[0] == 127 && adr->ip[1] == 0 && adr->ip[2] == 0 && adr->ip[3] == 1;
}

// Returns true if we can't bind the address locally--in other words, the IP is NOT one of our interfaces.
bool NET_IsClientLegal (netadr_t *adr)
{
	struct sockaddr_in sadr;
	int newsocket;

	if (adr->ip[0] == 127)
		return true; // local connections are always valid

	NetadrToSockadr (adr, &sadr);

	newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (newsocket == -1)
	{
		Con_Printf ("NET_IsClientLegal: socket: %s\n", strerror (errno));
		return false;
	}

	sadr.sin_port = 0;

	if (bind (newsocket, (void *)&sadr, sizeof (sadr)) == -1)
	{
		// It is not a local address
		close (newsocket);
		return true;
	}

	close (newsocket);
	return false;
}

static bool NET_GetLoopbackPacket (netsocket_e sock)
{
	if (net_loopback_size[!sock] == 0)
		return false;

	memcpy (net_message_buffer[sock], net_loopback_buffer[!sock], net_loopback_size[!sock]);
	net_message[sock].cursize = net_loopback_size[!sock];
	net_loopback_size[!sock] = 0;

	net_from.ip[0] = 127;
	net_from.ip[1] = 0;
	net_from.ip[2] = 0;
	net_from.ip[3] = 1;
	net_from.port = (sock == SERVER) ? PORT_CLIENT : PORT_SERVER;
	return true;
}

bool NET_GetPacket (netsocket_e sock)
{
	if (NET_GetLoopbackPacket (sock))
		return true;

	if (net_socket[sock] == 0)
		return false;

	int ret;
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof (from);

	ret = recvfrom (net_socket[sock], net_message_buffer[sock], sizeof (net_message_buffer[sock]), 0, (struct sockaddr *)&from, &fromlen);

	if (ret == -1)
	{
		if (errno == EWOULDBLOCK)
			return false;
		if (errno == ECONNREFUSED)
			return false;
		Sys_Printf ("NET_GetPacket: %s\n", strerror (errno));
		return false;
	}

	net_message[sock].cursize = ret;
	SockadrToNetadr (&from, &net_from);

	return ret;
}

static bool NET_SendLoopbackPacket (netsocket_e sock, int length, void *data, netadr_t to)
{
	if (!NET_IsLocalHost (&to))
		return false;

	if (net_loopback_size[sock] + length <= MAX_UDP_PACKET)
	{
		memcpy (net_loopback_buffer[sock] + net_loopback_size[sock], data, length);
		net_loopback_size[sock] += length;
	}

	return true;
}

void NET_SendPacket (netsocket_e sock, int length, void *data, netadr_t to)
{
	if (NET_SendLoopbackPacket (sock, length, data, to))
		return;

	if (net_socket[sock] == 0)
		return;

	int ret;
	struct sockaddr_in addr;

	NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket[sock], data, length, 0, (struct sockaddr *)&addr, sizeof (addr));

	if (ret == -1)
	{
		if (errno == EWOULDBLOCK)
			return;
		if (errno == ECONNREFUSED)
			return;
		Sys_Printf ("NET_SendPacket: %s\n", strerror (errno));
	}
}

static int UDP_OpenSocket (int port)
{
	int newsocket;
	struct sockaddr_in address;
	bool _true = true;
	int i;

	newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newsocket == -1)
	{
		Con_Printf ("UDP_OpenSocket: socket: %s\n", strerror (errno));
		return 0;
	}

	if (ioctl (newsocket, FIONBIO, (char *)&_true) == -1)
	{
		Con_Printf ("UDP_OpenSocket: ioctl FIONBIO: %s\n", strerror (errno));
		close (newsocket);
		return 0;
	}

	address.sin_family = AF_INET;

	// ZOID -- check for interface binding option
	if ((i = COM_CheckParm ("-ip")) != 0 && i < com_argc)
	{
		address.sin_addr.s_addr = inet_addr (com_argv[i + 1]);
		Con_Printf ("Binding to IP Interface Address of %s\n", inet_ntoa (address.sin_addr));
	}
	else
		address.sin_addr.s_addr = INADDR_ANY;

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons (port);

	if (bind (newsocket, (void *)&address, sizeof (address)) == -1)
	{
		Con_Printf ("UDP_OpenSocket: bind: %s\n", strerror (errno));
		close (newsocket);
		return 0;
	}

	return newsocket;
}

netadr_t NET_GetLocalAddress (void)
{
	char buff[MAXHOSTNAMELEN];
	struct sockaddr_in address;
	int namelen;

	if (net_local_adr.ip[0])
		return net_local_adr;

	const bool is_open = net_socket[SERVER] != 0;
	if (!is_open)
	{
		if (!Host_InitServer ())
			return net_local_adr;
	}

	gethostname (buff, MAXHOSTNAMELEN);
	buff[MAXHOSTNAMELEN - 1] = 0;

	NET_StringToAdr (buff, &net_local_adr);

	namelen = sizeof (address);

	if (getsockname (net_socket[SERVER], (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", strerror (errno));

	net_local_adr.port = ntohs (address.sin_port);

	Con_Printf ("Local IP address: %s\n", NET_AdrToString (net_local_adr));

	if (!is_open)
		NET_Close (SERVER);

	return net_local_adr;
}

/*
====================
NET_GetPublicAddress

ping an api to get the host's public ip address
====================
*/
char *NET_GetPublicAddress (void)
{
#define PINGHOST "api.ipify.org"

	if (net_public_adr[0] != '\0')
		return net_public_adr;

	// use our local IP address in case something goes wrong
	strcpy (net_public_adr, NET_BaseAdrToString (net_local_adr));

#ifdef PINGHOST
	struct addrinfo hint;
	struct addrinfo *info;

	memset (&hint, 0, sizeof (hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	// get the address info
	if (getaddrinfo (PINGHOST, "http", &hint, &info) == 0 && info)
	{
		struct addrinfo *i;
		int remote = -1;

		// find a socket we like
		for (i = info; i; i = i->ai_next)
		{
			remote = socket (i->ai_family, i->ai_socktype, 0);

			if (remote != -1)
			{
				// make sure it's blocking
				int flags = fcntl (remote, F_GETFL, 0);
				flags &= ~O_NONBLOCK;
				fcntl (remote, F_SETFL, flags);

				// establish the connection!
				if (connect (remote, i->ai_addr, i->ai_addrlen) == 0)
					break;

				close (remote);
				remote = -1;
			}
		}

		freeaddrinfo (info);

		if (remote != -1)
		{
			char *request = "GET / HTTP/1.0\r\nHost: " PINGHOST "\r\nUser-Agent: Quake\r\n\r\n";
			int len = strlen (request);

			// send the request
			if (send (remote, request, len, 0) != len + 1)
			{
				char response[1024];

				// wait for the response
				len = recv (remote, response, sizeof (response) - 1, 0);

				if (len != -1)
				{
					response[len] = '\0';
					char *str = response;

					// ensure the response is ok and then jump to the end of the header
					if ((str = strstr (str, "200 OK")) && (str = strstr (str, "\r\n\r\n")))
					{
						// we got our address!
						strcpy (net_public_adr, str + 4);
					}
				}
			}

			close (remote);
		}
	}

#undef PINGHOST
#endif

	return net_public_adr;
}

void NET_Close (netsocket_e sock)
{
	if (net_socket[sock] != 0)
	{
		close (net_socket[sock]);
		net_socket[sock] = 0;
		Con_Printf ("%s socket closed\n", (sock == SERVER) ? "Server" : "Client");
	}
	net_loopback_size[sock] = 0;
}

bool NET_Open (netsocket_e sock, int port)
{
	if (net_socket[sock] != 0)
		return true;

	NET_Close (sock);

	//
	// open the communications socket
	//
	net_socket[sock] = UDP_OpenSocket (port);

	if (net_socket[sock] == 0)
		return false;

	Con_Printf ("%s socket opened\n", (sock == SERVER) ? "Server" : "Client");

	return true;
}

void NET_Init (void)
{
	//
	// init the message buffers
	//
	net_message[CLIENT].maxsize = sizeof (net_message_buffer[CLIENT]);
	net_message[CLIENT].data = net_message_buffer[CLIENT];

	net_message[SERVER].maxsize = sizeof (net_message_buffer[SERVER]);
	net_message[SERVER].data = net_message_buffer[SERVER];

	//
	// determine my name & address
	//
	NET_GetLocalAddress ();

	Con_Printf ("UDP Initialized\n");
}

void NET_Shutdown (void)
{
	NET_Close (SERVER);
	NET_Close (CLIENT);
}
