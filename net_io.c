// dump1090, a Mode S messages decoder for RTLSDR devices.
//
// Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "dump1090.h"
//
// ============================= Networking =============================
//
// Note: here we disregard any kind of good coding practice in favor of
// extreme simplicity, that is:
//
// 1) We only rely on the kernel buffers for our I/O without any kind of
//    user space buffering.
// 2) We don't register any kind of event handler, from time to time a
//    function gets called and we accept new connections. All the rest is
//    handled via non-blocking I/O and manually polling clients to see if
//    they have something new to share with us when reading is needed.
//
//=========================================================================
//
// Networking "stack" initialization
//
struct service {
	char *descr;
	int *socket;
	int port;
	int enabled;
};

struct service services[MODES_NET_SERVICES_NUM];

void modesInitNet(void) {
    int j;

	struct service svc[MODES_NET_SERVICES_NUM] = {
		{"Raw TCP output", &modes.ros, modes.net_output_raw_port, 1},
		{"Raw TCP input", &modes.ris, modes.net_input_raw_port, 1},
		{"Beast TCP output", &modes.bos, modes.net_output_beast_port, 1},
		{"Beast TCP input", &modes.bis, modes.net_input_beast_port, 1},
		{"HTTP server", &modes.https, modes.net_http_port, 1},
		{"Basestation TCP output", &modes.sbsos, modes.net_output_sbs_port, 1}
	};

	memcpy(&services, &svc, sizeof(svc));//services = svc;

    modes.clients = NULL;

#ifdef _WIN32
    if ( (!modes.wsaData.wVersion) 
      && (!modes.wsaData.wHighVersion) ) {
      // Try to start the windows socket support
      if (WSAStartup(MAKEWORD(2,1),&modes.wsaData) != 0) 
        {
        fprintf(stderr, "WSAStartup returned Error\n");
        }
      }
#endif

    for (j = 0; j < MODES_NET_SERVICES_NUM; j++) {
		services[j].enabled = (services[j].port != 0);
		if (services[j].enabled) {
			int s = anetTcpServer(modes.aneterr, services[j].port, modes.net_bind_address);
			if (s == -1) {
				fprintf(stderr, "Error opening the listening port %d (%s): %s\n",
					services[j].port, services[j].descr, modes.aneterr);
				exit(1);
			}
			anetNonBlock(modes.aneterr, s);
			*services[j].socket = s;
		} else {
			if (modes.debug & MODES_DEBUG_NET) printf("%s port is disabled\n", services[j].descr);
		}
    }

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
}
//
//=========================================================================
//
// This function gets called from time to time when the decoding thread is
// awakened by new data arriving. This usually happens a few times every second
//
struct client * modesAcceptClients(void) {
    int fd, port;
    unsigned int j;
    struct client *c;

    for (j = 0; j < MODES_NET_SERVICES_NUM; j++) {
		if (services[j].enabled) {
			fd = anetTcpAccept(modes.aneterr, *services[j].socket, NULL, &port);
			if (fd == -1) continue;

			anetNonBlock(modes.aneterr, fd);
			c = (struct client *) malloc(sizeof(*c));
			c->service    = *services[j].socket;
			c->next       = modes.clients;
			c->fd         = fd;
			c->buflen     = 0;
			modes.clients = c;
			anetSetSendBuffer(modes.aneterr,fd, (MODES_NET_SNDBUF_SIZE << modes.net_sndbuf_size));

			if (*services[j].socket == modes.sbsos) modes.stat_sbs_connections++;
			if (*services[j].socket == modes.ros)   modes.stat_raw_connections++;
			if (*services[j].socket == modes.bos)   modes.stat_beast_connections++;

			j--; // Try again with the same listening port

			if (modes.debug & MODES_DEBUG_NET)
				printf("Created new client %d\n", fd);
		}
    }
    return modes.clients;
}
//
//=========================================================================
//
// On error free the client, collect the structure, adjust maxfd if needed.
//
void modesFreeClient(struct client *c) {

    // Unhook this client from the linked list of clients
    struct client *p = modes.clients;
    if (p) {
        if (p == c) {
            modes.clients = c->next;
        } else {
            while ((p) && (p->next != c)) {
                p = p->next;
            }
            if (p) {
                p->next = c->next;
            }
        }
    }

    free(c);
}
//
//=========================================================================
//
// Close the client connection and mark it as closed
//
void modesCloseClient(struct client *c) {
	close(c->fd);
    if (c->service == modes.sbsos) {
        if (modes.stat_sbs_connections) modes.stat_sbs_connections--;
    } else if (c->service == modes.ros) {
        if (modes.stat_raw_connections) modes.stat_raw_connections--;
    } else if (c->service == modes.bos) {
        if (modes.stat_beast_connections) modes.stat_beast_connections--;
    }

    if (modes.debug & MODES_DEBUG_NET)
        printf("Closing client %d\n", c->fd);

    c->fd = -1;
}

//=========================================================================
//
// This function decodes a Beast binary format message
//
// The message is passed to the higher level layers, so it feeds
// the selected screen output, the network output and so forth.
//
// If the message looks invalid it is silently discarded.
//
// The function always returns 0 (success) to the caller as there is no
// case where we want broken messages here to close the client connection.
//
int decodeBinMessage(struct client *c, char *p) {
    int msgLen = 0;
    int  j;
    char ch;
    char * ptr;
    unsigned char msg[MODES_LONG_MSG_BYTES];
    struct modesMessage mm;
    MODES_NOTUSED(c);
    memset(&mm, 0, sizeof(mm));

    ch = *p++; /// Get the message type
    if (0x1A == ch) {p++;} 

    if       ((ch == '1') && (modes.mode_ac)) { // skip ModeA/C unless user enables --modes-ac
        msgLen = MODEAC_MSG_BYTES;
    } else if (ch == '2') {
        msgLen = MODES_SHORT_MSG_BYTES;
    } else if (ch == '3') {
        msgLen = MODES_LONG_MSG_BYTES;
    }

    if (msgLen) {
        // Mark messages received over the internet as remote so that we don't try to
        // pass them off as being received by this instance when forwarding them
        mm.remote      =    1;

        ptr = (char*) &mm.timestampMsg;
        for (j = 0; j < 6; j++) { // Grab the timestamp (big endian format)
            ptr[5-j] = ch = *p++; 
            if (0x1A == ch) {p++;}
        }

        mm.signalLevel = ch = *p++;  // Grab the signal level
        if (0x1A == ch) {p++;}

        for (j = 0; j < msgLen; j++) { // and the data
            msg[j] = ch = *p++;
            if (0x1A == ch) {p++;}
        }

        if (msgLen == MODEAC_MSG_BYTES) { // ModeA or ModeC
            decodeModeAMessage(&mm, ((msg[0] << 8) | msg[1]));
        } else {
            decodeModesMessage(&mm, msg);
        }

        useModesMessage(&mm);
    }
    return (0);
}
//
//=========================================================================
//
// Turn an hex digit into its 4 bit decimal value.
// Returns -1 if the digit is not in the 0-F range.
//
int hexDigitVal(int c) {
    c = tolower(c);
    if (c >= '0' && c <= '9') return c-'0';
    else if (c >= 'a' && c <= 'f') return c-'a'+10;
    else return -1;
}
//
//=========================================================================
//
// This function decodes a string representing message in raw hex format
// like: *8D4B969699155600E87406F5B69F; The string is null-terminated.
// 
// The message is passed to the higher level layers, so it feeds
// the selected screen output, the network output and so forth.
// 
// If the message looks invalid it is silently discarded.
//
// The function always returns 0 (success) to the caller as there is no 
// case where we want broken messages here to close the client connection.
//
int decodeHexMessage(struct client *c, char *hex) {
    int l = strlen(hex), j;
    unsigned char msg[MODES_LONG_MSG_BYTES];
    struct modesMessage mm;
    MODES_NOTUSED(c);
    memset(&mm, 0, sizeof(mm));

    // Mark messages received over the internet as remote so that we don't try to
    // pass them off as being received by this instance when forwarding them
    mm.remote      =    1;
    mm.signalLevel = 0xFF;

    // Remove spaces on the left and on the right
    while(l && isspace(hex[l-1])) {
        hex[l-1] = '\0'; l--;
    }
    while(isspace(*hex)) {
        hex++; l--;
    }

    // Turn the message into binary.
    // Accept *-AVR raw @-AVR/BEAST timeS+raw %-AVR timeS+raw (CRC good) <-BEAST timeS+sigL+raw
    // and some AVR records that we can understand
    if (hex[l-1] != ';') {return (0);} // not complete - abort

    switch(hex[0]) {
        case '<': {
            mm.signalLevel = (hexDigitVal(hex[13])<<4) | hexDigitVal(hex[14]);
            hex += 15; l -= 16; // Skip <, timestamp and siglevel, and ;
            break;}

        case '@':     // No CRC check
        case '%': {   // CRC is OK
            hex += 13; l -= 14; // Skip @,%, and timestamp, and ;
            break;}

        case '*':
        case ':': {
            hex++; l-=2; // Skip * and ;
            break;}

        default: {
            return (0); // We don't know what this is, so abort
            break;}
    }

    if ( (l != (MODEAC_MSG_BYTES      * 2)) 
      && (l != (MODES_SHORT_MSG_BYTES * 2)) 
      && (l != (MODES_LONG_MSG_BYTES  * 2)) )
        {return (0);} // Too short or long message... broken

    if ( (0 == modes.mode_ac) 
      && (l == (MODEAC_MSG_BYTES * 2)) ) 
        {return (0);} // Right length for ModeA/C, but not enabled

    for (j = 0; j < l; j += 2) {
        int high = hexDigitVal(hex[j]);
        int low  = hexDigitVal(hex[j+1]);

        if (high == -1 || low == -1) return 0;
        msg[j/2] = (high << 4) | low;
    }

    if (l == (MODEAC_MSG_BYTES * 2)) {  // ModeA or ModeC
        decodeModeAMessage(&mm, ((msg[0] << 8) | msg[1]));
    } else {       // Assume ModeS
        decodeModesMessage(&mm, msg);
    }

    useModesMessage(&mm);
    return (0);
}
//
//=========================================================================
//
// This function polls the clients using read() in order to receive new
// messages from the net.
//
// The message is supposed to be separated from the next message by the
// separator 'sep', which is a null-terminated C string.
//
// Every full message received is decoded and passed to the higher layers
// calling the function's 'handler'.
//
// The handler returns 0 on success, or 1 to signal this function we should
// close the connection with the client in case of non-recoverable errors.
//
void modesReadFromClient(struct client *c, char *sep,
                         int(*handler)(struct client *, char *)) {
    int left;
    int nread;
    int fullmsg;
    int bContinue = 1;
    char *s, *e, *p;

    while(bContinue) {

        fullmsg = 0;
        left = MODES_CLIENT_BUF_SIZE - c->buflen;
        // If our buffer is full discard it, this is some badly formatted shit
        if (left <= 0) {
            c->buflen = 0;
            left = MODES_CLIENT_BUF_SIZE;
            // If there is garbage, read more to discard it ASAP
        }
#ifndef _WIN32
        nread = read(c->fd, c->buf+c->buflen, left);
#else
        nread = recv(c->fd, c->buf+c->buflen, left, 0);
        if (nread < 0) {errno = WSAGetLastError();}
#endif
        if (nread == 0) {
			modesCloseClient(c);
			return;
		}

        // If we didn't get all the data we asked for, then return once we've processed what we did get.
        if (nread != left) {
            bContinue = 0;
        }
#ifndef _WIN32
        if ( (nread < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || nread == 0 ) { // Error, or end of file
#else
        if ( (nread < 0) && (errno != EWOULDBLOCK)) { // Error, or end of file
#endif
            modesCloseClient(c);
            return;
        }
        if (nread <= 0) {
            break; // Serve next client
        }
        c->buflen += nread;

        // Always null-term so we are free to use strstr() (it won't affect binary case)
        c->buf[c->buflen] = '\0';

        e = s = c->buf;                                // Start with the start of buffer, first message



        if (c->service == modes.bis) {
            // This is the Beast Binary scanning case.
            // If there is a complete message still in the buffer, there must be the separator 'sep'
            // in the buffer, note that we full-scan the buffer at every read for simplicity.

            left = c->buflen;                                  // Length of valid search for memchr()
            while (left && ((s = (char*)memchr(e, (char) 0x1a, left)) != NULL)) { // The first byte of buffer 'should' be 0x1a
                s++;                                           // skip the 0x1a
                if        (*s == '1') {
                    e = s + MODEAC_MSG_BYTES      + 8;         // point past remainder of message
                } else if (*s == '2') {
                    e = s + MODES_SHORT_MSG_BYTES + 8;
                } else if (*s == '3') {
                    e = s + MODES_LONG_MSG_BYTES  + 8;
                } else {
                    e = s;                                     // Not a valid beast message, skip
                    left = &(c->buf[c->buflen]) - e;
                    continue;
                }
                // we need to be careful of double escape characters in the message body
                for (p = s; p < e; p++) {
                    if (0x1A == *p) {
                        p++; e++;
                        if (e > &(c->buf[c->buflen])) {
                            break;
                        }
                    }
                }
                left = &(c->buf[c->buflen]) - e;
                if (left < 0) {                                // Incomplete message in buffer
                    e = s - 1;                                 // point back at last found 0x1a.
                    break;
                }
                // Have a 0x1a followed by 1, 2 or 3 - pass message less 0x1a to handler.
                if (handler(c, s)) {
                    modesCloseClient(c);
                    return;
                }
                fullmsg = 1;
            }
            s = e;     // For the buffer remainder below

        } else {
            //
            // This is the ASCII scanning case, AVR RAW or HTTP at present
            // If there is a complete message still in the buffer, there must be the separator 'sep'
            // in the buffer, note that we full-scan the buffer at every read for simplicity.
            //
            while ((e = strstr(s, sep)) != NULL) { // end of first message if found
                *e = '\0';                         // The handler expects null terminated strings
                if (handler(c, s)) {               // Pass message to handler.
                    modesCloseClient(c);           // Handler returns 1 on error to signal we .
                    return;                        // should close the client connection
                }
                s = e + strlen(sep);               // Move to start of next message
                fullmsg = 1;
            }
        }

        if (fullmsg) {                             // We processed something - so
            c->buflen = &(c->buf[c->buflen]) - s;  //     Update the unprocessed buffer length
            memmove(c->buf, s, c->buflen);         //     Move what's remaining to the start of the buffer
        } else {                                   // If no message was decoded process the next client
            break;
        }
    }
}
//
//=========================================================================
//
// Read data from clients. This function actually delegates a lower-level
// function that depends on the kind of service (raw, http, ...).
//
void modesReadFromClients(void) {

    struct client *c = modesAcceptClients();

    while (c) {
            // Read next before servicing client incase the service routine deletes the client! 
            struct client *next = c->next;

        if (c->fd >= 0) {
                modesReadFromClient(c,"",decodeBinMessage);
        } else {
            modesFreeClient(c);
        }
        c = next;
    }
}
//
// =============================== Network IO ===========================
//
