#include "AircraftLabel.h"
#include "Aircraft.h"

#include <algorithm> 

#include "SDL2/SDL2_gfxPrimitives.h"

using fmilliseconds = std::chrono::duration<float, std::milli>;

static std::chrono::high_resolution_clock::time_point now() {
    return std::chrono::high_resolution_clock::now();
}

static float elapsed(std::chrono::high_resolution_clock::time_point ref) {
            return (fmilliseconds {now() - ref}).count();
}

static float sign(float x) {
    return (x > 0) - (x < 0);
}

SDL_Rect AircraftLabel::getFullRect(int labelLevel) {
    SDL_Rect rect = {static_cast<int>(x),static_cast<int>(y),0,0};

   	SDL_Rect currentRect;

    if(labelLevel < 2) {
	   	currentRect = speedLabel.getRect();

        rect.w = std::max(rect.w,currentRect.w);  
        rect.h += currentRect.h; 
	}

    if(labelLevel < 1) {
	   	currentRect = altitudeLabel.getRect();

        rect.w = std::max(rect.w,currentRect.w);  
        rect.h += currentRect.h; 

	   	currentRect = speedLabel.getRect();

        rect.w = std::max(rect.w,currentRect.w);  
        rect.h += currentRect.h; 
	}

	return rect;
}

void AircraftLabel::update() {
    char flight[17] = "";
    snprintf(flight,17," %s", p->flight);


	std::string flightString = flight;
	flightString.erase(std::remove_if(flightString.begin(), flightString.end(), isspace), flightString.end());

    flightLabel.setText(flightString);

	char alt[10] = "";
    if (metric) {
        snprintf(alt,10," %dm", static_cast<int>(p->altitude / 3.2828)); 
    } else {
        snprintf(alt,10," %d'", p->altitude); 
    }

    altitudeLabel.setText(alt);

    char speed[10] = "";
    if (metric) {
        snprintf(speed,10," %dkm/h", static_cast<int>(p->speed * 1.852));
    } else {
        snprintf(speed,10," %dmph", p->speed);
    }

	speedLabel.setText(speed);
}

void AircraftLabel::clearAcceleration() {
	ddx = 0;
	ddy = 0;
}

float AircraftLabel::calculateDensity(Aircraft *check_p, int labelLevel) {
    float density_max = 0;

    while(check_p) {
        if(check_p->addr == p->addr) {
            check_p = check_p->next;
            continue;
        }

        if(!check_p->label) {
            check_p = check_p->next;
        	continue;
        }

     	if(check_p->label->x + check_p->label->w < 0) {
            check_p = check_p->next;
        	continue;
	    }

     	if(check_p->label->y + check_p->label->h < 0) {
            check_p = check_p->next;
        	continue;
	    }

     	if(check_p->label->x > screen_width) {
            check_p = check_p->next;
        	continue;
	    }

     	if(check_p->label->y > screen_height) {
            check_p = check_p->next;
        	continue;
	    }

        SDL_Rect currentRect = getFullRect(labelLevel);

        float width_proportion = (currentRect.w  + check_p->label->w) / fabs(x - check_p->label->x);
        float height_proportion = (currentRect.h + check_p->label->h) / fabs(y - check_p->label->y);

        float density = width_proportion * height_proportion;
        
        if(density > density_max) {
            density_max = density;
        }

        check_p = check_p -> next;
    }

    return density_max;
}

void AircraftLabel::calculateForces(Aircraft *check_p) {
	if(w == 0 || h == 0) {
		return;
	}

	Aircraft *head = check_p;

    int p_left = x;
    int p_right = x + w;
    int p_top = y;
    int p_bottom = y + h;

       
    float boxmid_x = static_cast<float>(p_left + p_right) / 2.0f;
    float boxmid_y = static_cast<float>(p_top + p_bottom) / 2.0f;
    
    float offset_x = boxmid_x - p->x;
    float offset_y = boxmid_y - p->y;

    float target_length_x = attachment_dist + w / 2.0f;
    float target_length_y = attachment_dist + h / 2.0f;

    // stay icon_dist away from own icon

    ddx -= sign(offset_x) * attachment_force * (fabs(offset_x) - target_length_x);
    ddy -= sign(offset_y) * attachment_force * (fabs(offset_y) - target_length_y);
    

    // screen edge 

    if(p_left < edge_margin) {
        ddx += boundary_force * static_cast<float>(edge_margin - p_left);
    }

    if(p_right > screen_width - edge_margin) {
        ddx += boundary_force * static_cast<float>(screen_width - edge_margin - p_right);
    }

    if(p_top < edge_margin) {
        ddy += boundary_force * static_cast<float>(edge_margin - p_top);
    }

    if(p_bottom > screen_height - edge_margin) {
        ddy += boundary_force * static_cast<float>(screen_height - edge_margin - p_bottom);
    }


    float all_x = 0;
    float all_y = 0;
    int count = 0;
    //check against other labels

    while(check_p) {
        if(check_p->addr == p->addr) {
            check_p = check_p->next;
            continue;
        }

        if(!check_p->label) {
            check_p = check_p->next;
        	continue;
        }

        int check_left = check_p->label->x;
        int check_right = check_p->label->x + check_p->label->w;
        int check_top = check_p->label->y;
        int check_bottom = check_p->label->y + check_p->label->h;

        float icon_x = static_cast<float>(check_p->x);
        float icon_y = static_cast<float>(check_p->y);

        float checkboxmid_x = static_cast<float>(check_left + check_right) / 2.0f;
        float checkboxmid_y = static_cast<float>(check_top + check_bottom) / 2.0f;

        float offset_x = boxmid_x - checkboxmid_x;
        float offset_y = boxmid_y - checkboxmid_y;

        float target_length_x = label_dist + static_cast<float>(check_p->label->w + w) / 2.0f;
        float target_length_y = label_dist + static_cast<float>(check_p->label->h + h) / 2.0f;
    
        float x_mag = std::max(0.0f,(target_length_x - fabs(offset_x)));
        float y_mag = std::max(0.0f,(target_length_y - fabs(offset_y)));

        // stay at least label_dist away from other icons

        if(x_mag > 0 && y_mag > 0) {
            ddx += sign(offset_x) * label_force * x_mag;                        
            ddy += sign(offset_y) * label_force * y_mag;    
        }            
   
        // stay at least icon_dist away from other icons

        offset_x = boxmid_x - check_p->x;
        offset_y = boxmid_y - check_p->y;

        target_length_x = icon_dist + static_cast<float>(check_p->label->w) / 2.0f;
        target_length_y = icon_dist + static_cast<float>(check_p->label->h) / 2.0f;
    
        x_mag = std::max(0.0f,(target_length_x - fabs(offset_x)));
        y_mag = std::max(0.0f,(target_length_y - fabs(offset_y)));

        if(x_mag > 0 && y_mag > 0) {
            ddx += sign(offset_x) * icon_force * x_mag;    
            ddy += sign(offset_y) * icon_force * y_mag;
        }

        all_x += sign(boxmid_x - checkboxmid_x);
        all_y += sign(boxmid_y - checkboxmid_y);

        count++;
    
        check_p = check_p -> next;
    }

    // move away from others
    ddx += density_force * all_x / count;
    ddy += density_force * all_y / count;

	// char buff[100];
	// snprintf(buff, sizeof(buff), "%2.2f", labelLevel);
	// debugLabel.setText(buff);

 	float density_mult = 0.5f;
 	float level_rate = 0.25f;

 	if(elapsed(lastLevelChange) > 1000.0) {
		if(labelLevel < 0.8f * density_mult * calculateDensity(head, labelLevel + 1)) {
		    if(labelLevel <= 2) {
		        labelLevel += level_rate;
		        lastLevelChange = now();
		    }
		} else if (labelLevel > 1.2f * density_mult * calculateDensity(head, labelLevel - 1)) {
		    if(labelLevel >= 0) {
		  		labelLevel -= level_rate;
		        lastLevelChange = now();
				}	                         
		}
 	}
}

void AircraftLabel::applyForces() {
        dx += ddx;
        dy += ddy;

        dx *= damping_force;
        dy *= damping_force;
  
        if(fabs(dx) > velocity_limit) {
            dx = sign(dx) * velocity_limit;
        }

        if(fabs(dy) > velocity_limit) {
            dy = sign(dy) * velocity_limit;
        }

        if(fabs(dx) < 0.01f) {
            dx = 0;
        }

        if(fabs(dy) < 0.01f) {
            dy = 0;
        }

        x += dx;
        y += dy;

        if(isnan(x)) {
        	x = 0;
        }

        if(isnan(y)) {
        	y = 0;
        }

        // x = p->cx + (int)round(p->ox);
        // y = p->cy + (int)round(p->oy);
}

// SDL_Color signalToColor(int signal) {
//     SDL_Color planeColor;

//     if(signal > 127) {
//         signal = 127;
//     }

//     if(signal < 0) {
//         planeColor = setColor(96, 96, 96);      
//     } else {
//         planeColor = setColor(parula[signal][0], parula[signal][1], parula[signal][2]);                 
//     }

//     return planeColor;
// }
// void View::drawSignalMarks(Aircraft *p, int x, int y) {
//     unsigned char * pSig       = p->signalLevel;
//     unsigned int signalAverage = (pSig[0] + pSig[1] + pSig[2] + pSig[3] + 
//                                               pSig[4] + pSig[5] + pSig[6] + pSig[7] + 3) >> 3; 

//     SDL_Color barColor = signalToColor(signalAverage);

//     Uint8 seenFade;

//     if(elapsed(p->msSeen) < 1024) {
//         seenFade = (Uint8) (255.0 - elapsed(p->msSeen) / 4.0);

//         circleRGBA(renderer, x + mapFontWidth, y - 5, 2 * screen_uiscale, barColor.r, barColor.g, barColor.b, seenFade);
//     }

//     if(elapsed(p->msSeenLatLon) < 1024) {
//         seenFade = (Uint8) (255.0 - elapsed(p->msSeenLatLon) / 4.0);

//         hlineRGBA(renderer, x + mapFontWidth + 5 * screen_uiscale, x + mapFontWidth + 9 * screen_uiscale, y - 5, barColor.r, barColor.g, barColor.b, seenFade);
//         vlineRGBA(renderer, x + mapFontWidth + 7 * screen_uiscale, y - 2 * screen_uiscale - 5, y + 2 * screen_uiscale - 5, barColor.r, barColor.g, barColor.b, seenFade);
//     }
// }


void AircraftLabel::draw(SDL_Renderer *renderer, bool selected) {
    if(x == 0 || y == 0) {
        return;
    }

	// char buff[100];
	// snprintf(buff, sizeof(buff), "%f %f", x, y);
	// debugLabel.setText(buff);

    int totalWidth = 0;
    int totalHeight = 0;

    // int margin = 4 * screen_uiscale;

	int margin = 4;

    SDL_Rect outRect;

    if(opacity == 0 && labelLevel < 2) {
        target_opacity = 1.0f;
    }

    if(opacity > 0 && labelLevel >= 2) {
        target_opacity = 0.0f;
    }

    opacity += 0.25f * (target_opacity - opacity);

    if(opacity < 0.05f) {
        opacity = 0;
    }

    if(w != 0 && h != 0 && opacity > 0) {

        SDL_Color drawColor = style.labelLineColor;

        drawColor.a = static_cast<int>(255.0f * opacity);

        if(selected) {
            drawColor = style.selectedColor;
        }

        int tick = 4;

        int anchor_x, anchor_y, exit_x, exit_y;

        if(x + w / 2 > p->x) {
            anchor_x = x;
        } else {
            anchor_x = x + w;
        }

        if(y + h / 2 > p->y) {
            anchor_y = y - margin;
        } else {
            anchor_y = y + h + margin;
        }

        if(abs(anchor_x - p->x) > abs(anchor_y - p->y)) {
            exit_x = (anchor_x + p->x) / 2;
            exit_y = anchor_y;
        } else {
            exit_x = anchor_x;
            exit_y = (anchor_y + p->y) / 2;            
        }

        Sint16 vx[3] = {
            static_cast<Sint16>(p->x), 
            static_cast<Sint16>(exit_x), 
            static_cast<Sint16>(anchor_x)};

        Sint16 vy[3] = {
            static_cast<Sint16>(p->y), 
            static_cast<Sint16>(exit_y), 
            static_cast<Sint16>(anchor_y)};        

        boxRGBA(renderer, x, y, x + w, y + h, 0, 0, 0, drawColor.a);

  //       char buff[100];
		// snprintf(buff, sizeof(buff), "%d", drawColor.a);
		// debugLabel.setText(buff);

        bezierRGBA(renderer, vx, vy, 3, 2, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        //lineRGBA(renderer, x,y - margin, x + tick, y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x,y - margin, x + w, y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x,y - margin, x, y - margin + tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        // lineRGBA(renderer, x + w, y - margin, x + w - tick, y - margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x + w, y - margin, x + w, y - margin + tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        //lineRGBA(renderer, x, y + h + margin, x + tick, y + h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x, y + h + margin, x + w, y + h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x, y + h + margin, x, y + h + margin - tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a);

        // lineRGBA(renderer, x + w, y + h + margin,x + w - tick, y + h + margin, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        lineRGBA(renderer, x + w, y + h + margin,x + w, y + h + margin - tick, drawColor.r, drawColor.g, drawColor.b, drawColor.a); 
    }
   
    if(labelLevel < 2 || selected) {
        // drawSignalMarks(p, x, y);

        SDL_Color drawColor = style.labelColor;
        drawColor.a = static_cast<int>(255.0f * opacity);

        flightLabel.setColor(drawColor);
        flightLabel.setPosition(x,y);
    	flightLabel.draw(renderer);
        // outRect = drawString(flight, x, y, mapBoldFont, drawColor); 
    	outRect = flightLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;            
    
    }

    if(labelLevel < 1 || selected) {
        SDL_Color drawColor = style.subLabelColor;
        drawColor.a = static_cast<int>(255.0f * opacity);

        altitudeLabel.setColor(drawColor);
        altitudeLabel.setPosition(x,y + totalHeight);
		altitudeLabel.draw(renderer);
    	outRect = altitudeLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;                              

        speedLabel.setColor(drawColor);
        speedLabel.setPosition(x,y + totalHeight);
		speedLabel.draw(renderer);
    	outRect = speedLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;      
    
    }

    debugLabel.setPosition(x,y + totalHeight);
    debugLabel.draw(renderer);

    target_w = totalWidth;
    target_h = totalHeight;

    w += 0.25f * (target_w - w);
    h += 0.25f * (target_h - h);

    if(w < 0.05f) {
        w = 0;
    }

    if(h < 0.05f) {
        h = 0;
    }	
}

AircraftLabel::AircraftLabel(Aircraft *p, bool metric, int screen_width, int screen_height, TTF_Font *font) {
	this->p = p;

	this->metric = metric;

	x = p->x;
    y = p->y + 20; //*screen_uiscale
    w = 0;
    h = 0;
    target_w = 0;
    target_h = 0;

    opacity = 0;
    target_opacity = 0;

    dx = 0;
    dy  = 0;
    ddx = 0;
    ddy = 0;

    this->screen_width = screen_width;
    this->screen_height = screen_height;

    labelLevel = 0;

    flightLabel.setFont(font);
	altitudeLabel.setFont(font);
	speedLabel.setFont(font);
	debugLabel.setFont(font);

	lastLevelChange = now();
}