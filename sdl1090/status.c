#include "dump1090.h"
#include "structs.h"
#include "parula.h"
#include "monokai.h"
#include "SDL/SDL_gfxPrimitives.h"

#define PAD 5

void updateStatus() {
	struct aircraft *a = Modes.aircrafts;

    int numVisiblePlanes = 0;
    double maxDist = 0;
    int totalCount = 0;
    double sigAccumulate = 0.0;
    double msgRateAccumulate = 0.0;    

    Status.closeCall = NULL;

    while(a) {
        int flags = a->modeACflags;
        int msgs  = a->messages;

        if ( (((flags & (MODEAC_MSG_FLAG                             )) == 0                    )                 )
           || (((flags & (MODEAC_MSG_MODES_HIT | MODEAC_MSG_MODEA_ONLY)) == MODEAC_MSG_MODEA_ONLY) && (msgs > 4  ) ) 
           || (((flags & (MODEAC_MSG_MODES_HIT | MODEAC_MSG_MODEC_OLD )) == 0                    ) && (msgs > 127) ) 
           ) {

            unsigned char * pSig       = a->signalLevel;
            unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
                                          pSig[4] + pSig[5] + pSig[6] + pSig[7]);   

			sigAccumulate += signalAverage;

            if (a->bFlags & MODES_ACFLAGS_LATLON_VALID) {
                double d = sqrt(a->dx * a->dx + a->dy * a->dy);

                if(d < LOGMAXDIST) {
	                if(d > maxDist) {
	                	maxDist = d;
	                }

	                if(d < 4.0) {
	                	Status.closeCall = a;
	                }

	                numVisiblePlanes++;
	            }
            }
            totalCount++;
        }


         msgRateAccumulate += (a->messageRate[0] + a->messageRate[1] + a->messageRate[2] + a->messageRate[3] + 
                                           a->messageRate[4] + a->messageRate[5] + a->messageRate[6] + a->messageRate[7]);   

        a = a->next;
    }

    Status.msgRate                = msgRateAccumulate;
    Status.avgSig                 = sigAccumulate / (double) totalCount;
    Status.numPlanes              = totalCount;
    Status.numVisiblePlanes    	  = numVisiblePlanes;
    Status.maxDist                = maxDist;
}

void drawStatusBox(int *left, int *top, char *label, char *message, SDL_Color color) {
	int labelWidth = ((strlen(label) > 0 ) ? 1.5 : 0) * game.labelFontHeight;
	int messageWidth = (strlen(message) + ((strlen(message) > 0 ) ? 1 : 0)) * game.messageFontWidth;

	//newline if no message or label
	if(strlen(label) == 0 && strlen(message) == 0 ) {
		boxRGBA(game.screen, *left, *top, Modes.screen_width - PAD, *top + game.messageFontHeight,0, 0, 0, 0);
		*left = PAD;
		*top = *top - game.messageFontHeight - PAD;		
		return;
	}	

	if(*left + labelWidth + messageWidth + PAD > Modes.screen_width) {
		// if(*left + PAD < Modes.screen_width) {
		// 	boxRGBA(game.screen, *left, *top, Modes.screen_width - PAD, *top + game.messageFontHeight, darkGrey.r, darkGrey.g, darkGrey.b, SDL_ALPHA_OPAQUE);
		// }
		*left = PAD;
		*top = *top - game.messageFontHeight - PAD;
	}

	// filled black background
	if(messageWidth) {
		boxRGBA(game.screen, *left, *top, *left + labelWidth + messageWidth, *top + game.messageFontHeight, black.r, black.g, black.b, SDL_ALPHA_OPAQUE);
	}

	// filled label box
	if(labelWidth) {
		boxRGBA(game.screen, *left, *top, *left + labelWidth, *top + game.messageFontHeight, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	}

	// outline message box
	if(messageWidth) {
		rectangleRGBA(game.screen, *left, *top, *left + labelWidth + messageWidth, *top + game.messageFontHeight, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	}

	// label
	drawString90(label, *left, *top + game.labelFontWidth/2, game.labelFont, black);

	//message
	drawString(message, *left + labelWidth + game.messageFontWidth/2, *top, game.messageFont, color);

	*left = *left + labelWidth + messageWidth + PAD;
}

void drawStatus() {

	int left = PAD;	
	int	top = Modes.screen_height - game.messageFontHeight - PAD;

	char strLoc[20] = " ";
    snprintf(strLoc, 20, "%3.3fN %3.3f%c", Modes.fUserLat, fabs(Modes.fUserLon),(Modes.fUserLon > 0) ? 'E' : 'W');
	drawStatusBox(&left, &top, "GPS", strLoc, pink);	

    char strPlaneCount[10] = " ";
    snprintf(strPlaneCount, 10,"%d/%d", Status.numVisiblePlanes, Status.numPlanes);
	drawStatusBox(&left, &top, "disp", strPlaneCount, yellow);

    char strDMax[5] = " ";
    snprintf(strDMax, 5, "%.0fkm", Status.maxDist);
	drawStatusBox(&left, &top, "mDst", strDMax, blue);

    char strMsgRate[18] = " ";
    snprintf(strMsgRate, 18,"%.0f/s", Status.msgRate);
  	drawStatusBox(&left, &top, "rate", strMsgRate, orange);

    char strSig[18] = " ";
    snprintf(strSig, 18, "%.0f%%", 100.0 * Status.avgSig / 1024.0);
  	drawStatusBox(&left, &top, "sAvg", strSig, green);

	drawStatusBox(&left, &top, "||||", "MENU", grey);

	if(Status.closeCall != NULL) {
		// if no flight id, "near" box goes on first line
		if(!strlen(Status.closeCall->flight)) {
			drawStatusBox(&left, &top, "near", "", red);				
		}			

	    char strSpeed[8] = " ";
	    snprintf(strSpeed, 8, "%.0fkm/h", Status.closeCall->speed * 1.852);
		drawStatusBox(&left, &top, "vel", strSpeed, white);					

	    char strAlt[8] = " ";
	    snprintf(strAlt, 8, "%.0fm", Status.closeCall->altitude / 3.2828);
		drawStatusBox(&left, &top, "alt", strAlt, white);			

		drawStatusBox(&left, &top, "", "", black);	//this is effectively a newline	

		if(strlen(Status.closeCall->flight)) {
			drawStatusBox(&left, &top, "near", "", red);				
			drawStatusBox(&left, &top, "id", Status.closeCall->flight, white);		
		}
	}
}
