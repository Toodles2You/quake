
#ifndef _NET_H
#define _NET_H

#define PORT_ANY -1

typedef enum
{
	CLIENT,
	SERVER,
	NUM_SOCKETS,
} netsocket_e;

typedef struct
{
	byte ip[4];
	uint16_t port;
	uint16_t pad;
} netadr_t;

extern netadr_t net_from; // address of who sent the packet
extern sizebuf_t net_message[NUM_SOCKETS];

extern cvar_t hostname;

extern int net_socket[NUM_SOCKETS];

void NET_Init (void);
void NET_Shutdown (void);
bool NET_GetPacket (netsocket_e sock);
void NET_SendPacket (netsocket_e sock, int length, void *data, netadr_t to);
void NET_Open (netsocket_e sock, int port);
void NET_Close (netsocket_e sock);
netadr_t NET_GetLocalAddress (void);
char *NET_GetPublicAddress (void);

bool NET_CompareAdr (netadr_t a, netadr_t b);
bool NET_CompareBaseAdr (netadr_t a, netadr_t b);
char *NET_AdrToString (netadr_t a);
char *NET_BaseAdrToString (netadr_t a);
bool NET_StringToAdr (char *s, netadr_t *a);
bool NET_IsClientLegal (netadr_t *adr);

//============================================================================

#define OLD_AVG 0.99 // total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define MAX_LATENT 32

typedef struct
{
	bool fatal_error;

	float last_received; // for timeouts

	// the statistics are cleared at each client begin, because
	// the server connecting process gives a bogus picture of the data
	float frame_latency; // rolling average
	float frame_rate;

	int drop_count; // dropped packets, cleared each level
	int good_count; // cleared each level

	netadr_t remote_address;
	netsocket_e socket;
	int qport;

	// bandwidth estimator
	double cleartime; // if realtime > nc->cleartime, free to go
	double rate;	  // seconds / byte

	// sequencing variables
	int incoming_sequence;
	int incoming_acknowledged;
	int incoming_reliable_acknowledged; // single bit

	int incoming_reliable_sequence; // single bit, maintained local

	int outgoing_sequence;
	int reliable_sequence;		// single bit
	int last_reliable_sequence; // sequence number of last send

	// reliable staging and holding areas
	sizebuf_t message; // writing buffer to send to server
	byte message_buf[MAX_MSGLEN];

	int reliable_length;
	byte reliable_buf[MAX_MSGLEN]; // unacked reliable message

	// time and size data to calculate bandwidth
	int outgoing_size[MAX_LATENT];
	double outgoing_time[MAX_LATENT];
} netchan_t;

extern int net_drop; // packets dropped before this one

void Netchan_Init (void);
void Netchan_Transmit (netchan_t *chan, int length, byte *data);
void Netchan_OutOfBand (netsocket_e sock, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint (netsocket_e sock, netadr_t adr, char *format, ...);
bool Netchan_Process (netchan_t *chan);
void Netchan_Setup (netchan_t *chan, netadr_t adr, netsocket_e sock, int qport);

bool Netchan_CanPacket (netchan_t *chan);
bool Netchan_CanReliable (netchan_t *chan);

#endif /* !_NET_H */
