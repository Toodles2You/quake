
#ifndef _NET_DGRM_H
#define _NET_DGRM_H

int Datagram_Init();
void Datagram_Listen(bool state);
void Datagram_SearchForHosts(bool xmit);
qsocket_t *Datagram_Connect(char *host);
qsocket_t *Datagram_CheckNewConnections();
int Datagram_GetMessage(qsocket_t *sock);
int Datagram_SendMessage(qsocket_t *sock, sizebuf_t *data);
int Datagram_SendUnreliableMessage(qsocket_t *sock, sizebuf_t *data);
bool Datagram_CanSendMessage(qsocket_t *sock);
bool Datagram_CanSendUnreliableMessage(qsocket_t *sock);
void Datagram_Close(qsocket_t *sock);
void Datagram_Shutdown();

#endif /* !_NET_DGRM_H */
