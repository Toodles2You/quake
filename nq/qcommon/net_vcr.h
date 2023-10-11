
#ifndef _NET_VCR_H
#define _NET_VCR_H

enum
{
    VCR_OP_CONNECT = 1,
    VCR_OP_GETMESSAGE,
    VCR_OP_SENDMESSAGE,
    VCR_OP_CANSENDMESSAGE,

    VCR_MAX_MESSAGE = VCR_OP_CANSENDMESSAGE,
};

int VCR_Init();
void VCR_Listen(bool state);
void VCR_SearchForHosts(bool xmit);
qsocket_t *VCR_Connect(char *host);
qsocket_t *VCR_CheckNewConnections();
int VCR_GetMessage(qsocket_t *sock);
int VCR_SendMessage(qsocket_t *sock, sizebuf_t *data);
bool VCR_CanSendMessage(qsocket_t *sock);
void VCR_Close(qsocket_t *sock);
void VCR_Shutdown();

#endif /* !_NET_VCR_H */
