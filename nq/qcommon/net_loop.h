
#ifndef _NET_LOOP_H
#define _NET_LOOP_H

int Loop_Init();
void Loop_Listen(bool state);
void Loop_SearchForHosts(bool xmit);
qsocket_t *Loop_Connect(char *host);
qsocket_t *Loop_CheckNewConnections();
int Loop_GetMessage(qsocket_t *sock);
int Loop_SendMessage(qsocket_t *sock, sizebuf_t *data);
int Loop_SendUnreliableMessage(qsocket_t *sock, sizebuf_t *data);
bool Loop_CanSendMessage(qsocket_t *sock);
bool Loop_CanSendUnreliableMessage(qsocket_t *sock);
void Loop_Close(qsocket_t *sock);
void Loop_Shutdown();

#endif /* !_NET_LOOP_H */
