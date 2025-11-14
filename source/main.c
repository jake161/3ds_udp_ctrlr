#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <3ds.h>
#include <ver.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;
s32 sock = -1;

__attribute__((format(printf,1,2)))
void failExit(const char *fmt, ...);

int main(int argc, char **argv)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

	char keysNames[32][32] = {
		"KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
		"KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
		"KEY_R", "KEY_L", "KEY_X", "KEY_Y",
		"", "", "KEY_ZL", "KEY_ZR",
		"", "", "", "",
		"KEY_TOUCH", "", "", "",
		"KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
		"KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
	};

    int ret;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);
    char temp[1024];
    int client_ready = 0; // flag when client has connected

    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if(!SOC_buffer) failExit("\x1b[1;1Hmemalign failed");

    if((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0){
        failExit("\x1b[1;1HsocInit: 0x%08X", (unsigned int)ret);
	}

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0) failExit("\x1b[1;1Hsocket: %d %s\n\n -erm... is the wifi connected?\n", errno, strerror(errno));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);       // server port
    server.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *)&server, sizeof(server))){
        failExit("\x1b[1;1Hbind: %d %s", errno, strerror(errno));
	}

    u32 ip = gethostid();
    printf("\x1b[1;1H3DS UDP server running on %lu.%lu.%lu.%lu:%d",
            (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF,
            ntohs(server.sin_port));
	printf("\x1b[25;1HVer. %s", APP_VERSION);
    printf("\x1b[29;17HStart+R+DOWN   | exit");
	printf("\x1b[30;17HSelect+R+DOWN  | Reset Client");

	u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0;

    while (aptMainLoop())
    {
		gspWaitForVBlank();
        hidScanInput();
        if((hidKeysHeld() & (KEY_START | KEY_R | KEY_DDOWN)) == (KEY_START | KEY_R | KEY_DDOWN)) break;
        if((hidKeysHeld() & (KEY_SELECT | KEY_R | KEY_DDOWN)) == (KEY_SELECT | KEY_R | KEY_DDOWN)) client_ready = 0;

        circlePosition pos;
        hidCircleRead(&pos);

        // wait for a client packet if we don't know client yet
        if(!client_ready) {
            int n = recvfrom(sock, temp, sizeof(temp)-1, MSG_DONTWAIT,
                             (struct sockaddr*)&client, &clientlen);
            printf("\x1b[2;1HClient not connected");

            if(n > 0) {
                temp[n] = 0;
                client_ready = 1;
            }
        }

		u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp   = hidKeysUp();

        // display locally
		
        // consoleClear();
		printf("\x1b[1;1H3DS UDP server running on %lu.%lu.%lu.%lu:%d",
            (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF,
            ntohs(server.sin_port));
		client_ready < 1 ? printf("\x1b[2;1HClient not connected") : printf("\x1b[2;1HClient connected from %s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("\x1b[5;1HCirclePad position:");
		printf("\x1b[6;1H%04d; %04d", pos.dx, pos.dy);
        printf("\x1b[25;1HVer. %s", APP_VERSION);
        printf("\x1b[29;17HStart+R+DOWN   | exit");
		printf("\x1b[30;17HSelect+R+DOWN  | Reset Client");

		if (kDown != kDownOld || kHeld != kHeldOld || kUp != kUpOld)
		{

            // send JSON to client if connected
            if(client_ready) {
                snprintf(temp, sizeof(temp),"{\"dx\":%d,\"dy\":%d,\"kDown\":%lu,\"kHeld\":%lu,\"kUp\":%lu}\n", pos.dx, pos.dy, kDown, kHeld, kUp);
                sendto(sock, temp, strlen(temp), 0, (struct sockaddr*)&client, clientlen);
            }
            
			//Clear console
			consoleClear();

			//These two lines must be rewritten because we cleared the whole console
			printf("\x1b[5;1HCirclePad position:");
			printf("\x1b[6;1H%04d; %04d", pos.dx, pos.dy);
			printf("\x1b[7;1H");
			

			//Check if some of the keys are down, held or up
			int i;
			for (i = 0; i < 32; i++)
			{
				if (kDown & BIT(i)) printf("%s down\n", keysNames[i]);
				if (kHeld & BIT(i)) printf("%s held\n", keysNames[i]);
				if (kUp & BIT(i)) printf("%s up\n", keysNames[i]);
			}
		}

		//Set keys old values for the next frame
		kDownOld = kDown;
		kHeldOld = kHeld;
		kUpOld = kUp;

        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    close(sock);
    socExit();
    gfxExit();
    return 0;
}

void failExit(const char *fmt, ...) {
    if(sock>0) close(sock);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\nPress B to exit\n");

    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        if(hidKeysDown() & KEY_B) exit(0);
    }
}
