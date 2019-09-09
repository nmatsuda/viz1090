#include "dump1090.h"
#include "structs.h"
#include "parula.h"
#include "monokai.h"
#include "SDL2/SDL2_gfxPrimitives.h"

#define PAD 5

void updateStatus() {
	// struct aircraft *a = Modes.aircrafts;

    int numVisiblePlanes = 0;
    double maxDist = 0;
    int totalCount = 0;
    double sigAccumulate = 0.0;
    double msgRateAccumulate = 0.0;    

/*
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

                if(d < appData.maxDist) {
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
*/
    struct planeObj *p = planes;

    while(p) {
		unsigned char * pSig       = p->signalLevel;
		unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
		                              pSig[4] + pSig[5] + pSig[6] + pSig[7]);   

		sigAccumulate += signalAverage;
		
		if (p->lon && p->lat) {


			//distance measurements got borked during refactor - need to redo here
			/*
		    double d = sqrt(p->dx * a->dx + a->dy * a->dy);

		    if(d < appData.maxDist) {
		        if(d > maxDist) {
		        	maxDist = d;
		        }
			*/
		        numVisiblePlanes++;
		    //}
		}
		

		totalCount++;

        msgRateAccumulate += p->messageRate; 

        p = p->next;
    }

    Status.msgRate                = msgRateAccumulate;
    Status.avgSig                 = sigAccumulate / (double) totalCount;
    Status.numPlanes              = totalCount;
    Status.numVisiblePlanes    	  = numVisiblePlanes;
    Status.maxDist                = maxDist;
}

void drawStatusBox(int *left, int *top, char *label, char *message, SDL_Color color) {
	//int labelWidth = ((strlen(label) > 0 ) ? 1.5 : 0) * appData.labelFont;
	int labelWidth = (strlen(label) + ((strlen(label) > 0 ) ? 1 : 0)) * appData.labelFontWidth;
	int messageWidth = (strlen(message) + ((strlen(message) > 0 ) ? 1 : 0)) * appData.messageFontWidth;

	//newline if no message or label
	if(strlen(label) == 0 && strlen(message) == 0 ) {
		boxRGBA(appData.renderer, *left, *top, appData.screen_width - PAD, *top + appData.messageFontHeight,0, 0, 0, 0);
		*left = PAD;
		*top = *top - appData.messageFontHeight - PAD;		
		return;
	}	

	if(*left + labelWidth + messageWidth + PAD > appData.screen_width) {
		// if(*left + PAD < appData.screen_width) {
		// 	boxRGBA(appData.screen, *left, *top, appData.screen_width - PAD, *top + appData.messageFontHeight, darkGrey.r, darkGrey.g, darkGrey.b, SDL_ALPHA_OPAQUE);
		// }
		*left = PAD;
		*top = *top - appData.messageFontHeight - PAD;
	}

	// filled black background
	if(messageWidth) {
		roundedBoxRGBA(appData.renderer, *left, *top, *left + labelWidth + messageWidth, *top + appData.messageFontHeight, ROUND_RADIUS, black.r, black.g, black.b, SDL_ALPHA_OPAQUE);
	}

	// filled label box
	if(labelWidth) {
		roundedBoxRGBA(appData.renderer, *left, *top, *left + labelWidth, *top + appData.messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	}

	// outline message box
	if(messageWidth) {
		roundedRectangleRGBA(appData.renderer, *left, *top, *left + labelWidth + messageWidth, *top + appData.messageFontHeight, ROUND_RADIUS,color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
	}

	// label
	//drawString90(label, *left, *top + appData.labelFontWidth/2, appData.labelFont, black);
	
	drawString(label, *left + appData.labelFontWidth/2, *top, appData.labelFont, black);

	//message
	drawString(message, *left + labelWidth + appData.messageFontWidth/2, *top, appData.messageFont, color);

	*left = *left + labelWidth + messageWidth + PAD;
}



void drawButtonBox(int *left, int *top, char *label, SDL_Color color) {
	int labelWidth = (strlen(label) + ((strlen(label) > 0 ) ? 1 : 0)) * appData.labelFontWidth;

	//newline if no message or label
	if(strlen(label) == 0) {
		boxRGBA(appData.renderer, *left, *top, appData.screen_width - PAD, *top + appData.messageFontHeight,0, 0, 0, 0);
		*left = PAD;
		*top = *top - appData.messageFontHeight - PAD;		
		return;
	}	

	if(*left + labelWidth + PAD > appData.screen_width) {
		*left = PAD;
		*top = *top - appData.messageFontHeight - PAD;
	}

	// outline message box
	if(labelWidth) {

		roundedRectangleRGBA(appData.renderer, *left, *top , *left + labelWidth - 1, *top + appData.messageFontHeight - 1, ROUND_RADIUS, 255, 255, 255, SDL_ALPHA_OPAQUE);
		roundedRectangleRGBA(appData.renderer, *left + 1, *top + 1, *left + labelWidth , *top + appData.messageFontHeight, ROUND_RADIUS, 20, 20, 20, SDL_ALPHA_OPAQUE);
		roundedBoxRGBA(appData.renderer, *left + 1, *top + 1, *left + labelWidth - 1, *top + appData.messageFontHeight - 1, ROUND_RADIUS, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);

	}

	drawString(label, *left + appData.labelFontWidth/2, *top, appData.labelFont, black);

	*left = *left + labelWidth + PAD;
}



void drawBattery(int *left, int *top, double level) {
	int lineWidth = 1;

	int pointCount = 9;
	float xList[9] = {0.0, 0.25, 0.25, 0.75, 0.75, 1.0, 1.0, 0.0, 0.0};
	float yList[9] = {0.2, 0.2, 0.0, 0.0, 0.2, 0.2, 1.0, 1.0, 0.2};	

	for(int k = 0; k < pointCount - 1; k++) {
	    thickLineRGBA(appData.renderer, 
	    	*left + appData.messageFontWidth * xList[k], 
	    	*top + appData.messageFontHeight * yList[k], 
	    	*left + appData.messageFontWidth * xList[k+1], 
	    	*top + appData.messageFontHeight * yList[k+1], 
	    	lineWidth, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);
	}

	boxRGBA(appData.renderer, *left, *top + (0.2 + 0.8  * (1.0 - level)) * appData.messageFontHeight, *left + appData.messageFontWidth, *top + appData.messageFontHeight, grey.r, grey.g, grey.b, SDL_ALPHA_OPAQUE);

	*left = *left + appData.messageFontWidth;
}

void drawStatus() {

	int left = PAD + 2 * appData.messageFontHeight ;	
	int	top = appData.screen_height - 2 * appData.messageFontHeight - PAD;

	char strLoc[20] = " ";
    snprintf(strLoc, 20, "%3.3fN %3.3f%c", appData.centerLat, fabs(appData.centerLon),(appData.centerLon > 0) ? 'E' : 'W');
	drawStatusBox(&left, &top, "loc", strLoc, pink);	

	// drawBattery(&left, &top, 0.85);

    char strPlaneCount[10] = " ";
    snprintf(strPlaneCount, 10,"%d/%d", Status.numVisiblePlanes, Status.numPlanes);
	drawStatusBox(&left, &top, "disp", strPlaneCount, yellow);

	//distance measurements got borked during refactor - need to redo here

 //    char strDMax[5] = " ";
 //    snprintf(strDMax, 5, "%.0fkm", Status.maxDist);
	// drawStatusBox(&left, &top, "mDst", strDMax, blue);

    char strMsgRate[18] = " ";
    snprintf(strMsgRate, 18,"%.0f/s", Status.msgRate);
  	drawStatusBox(&left, &top, "rate", strMsgRate, orange);

    char strSig[18] = " ";
    snprintf(strSig, 18, "%.0f%%", 100.0 * Status.avgSig / 1024.0);
  	drawStatusBox(&left, &top, "sAvg", strSig, green);

	// drawStatusBox(&left, &top, "||||", "MENU", grey);

	// if(Status.closeCall != NULL) {
	// 	drawStatusBox(&left, &top, "", "", black);	//this is effectively a newline						
	// 	if(strlen(Status.closeCall->flight)) {	
	// 		drawStatusBox(&left, &top, "near", Status.closeCall->flight, white);		
	// 	} else {
	// 		drawStatusBox(&left, &top, "near", "", white);				
	// 	}
	// }
}
