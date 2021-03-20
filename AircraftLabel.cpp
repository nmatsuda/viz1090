#include "AircraftLabel.h"
#include "Aircraft.h"

#include "SDL2/SDL2_gfxPrimitives.h"


static float sign(float x) {
    return (x > 0) - (x < 0);
}

void AircraftLabel::update() {
    char flight[10] = "";
    snprintf(flight,10," %s", p->flight);

    flightLabel.setText(flight);

	char alt[10] = "";
    if (metric) {
        snprintf(alt,10," %dm", (int) (p->altitude / 3.2828)); 
    } else {
        snprintf(alt,10," %d'", p->altitude); 
    }

    altitudeLabel.setText(alt);

    char speed[10] = "";
    if (metric) {
        snprintf(speed,10," %dkm/h", (int) (p->speed * 1.852));
    } else {
        snprintf(speed,10," %dmph", p->speed);
    }

	speedLabel.setText(speed);
}

void AircraftLabel::clearAcceleration() {
	ddx = 0;
	ddy = 0;
}

void AircraftLabel::calculateForces(Aircraft *check_p) {
    int p_left = x;
    int p_right = x + w;
    int p_top = y;
    int p_bottom = y + h;

       
    float boxmid_x = (float)(p_left + p_right) / 2.0f;
    float boxmid_y = (float)(p_top + p_bottom) / 2.0f;
    
    float offset_x = boxmid_x - p->x;
    float offset_y = boxmid_y - p->y;

    float target_length_x = attachment_dist + w / 2.0f;
    float target_length_y = attachment_dist + h / 2.0f;

    // stay icon_dist away from own icon

    ddx -= sign(offset_x) * attachment_force * (fabs(offset_x) - target_length_x);
    ddy -= sign(offset_y) * attachment_force * (fabs(offset_y) - target_length_y);
    

    // // //screen edge 

    if(p_left < edge_margin) {
        ddx += boundary_force * (float)(edge_margin - p_left);
    }

    if(p_right > screen_width - edge_margin) {
        ddx += boundary_force * (float)(screen_width - edge_margin - p_right);
    }

    if(p_top < edge_margin) {
        ddy += boundary_force * (float)(edge_margin - p_top);
    }

    if(p_bottom > screen_height - edge_margin) {
        ddy += boundary_force * (float)(screen_height - edge_margin - p_bottom);
    }


    float all_x = 0;
    float all_y = 0;
    int count = 0;
    //check against other labels

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

        //calculate density for label display level (inversely proportional to area of smallest box connecting this to neighbor)
        float density = 1.0 / (0.001f + fabs(x - check_p->label->x) * fabs (x - check_p->label->y));

        if(density > density_max) {
            density_max = density;
        }

        density = 1.0 / (0.001f + fabs(x - check_p->x) * fabs(x - check_p->y));
        
        if(density > density_max) {
            density_max = density;
        }

        int check_left = check_p->label->x;
        int check_right = check_p->label->x + check_p->label->w;
        int check_top = check_p->label->y;
        int check_bottom = check_p->label->y + check_p->label->h;

        float icon_x = (float)check_p->x;
        float icon_y = (float)check_p->y;

        float checkboxmid_x = (float)(check_left + check_right) / 2.0f;
        float checkboxmid_y = (float)(check_top + check_bottom) / 2.0f;

        float offset_x = boxmid_x - checkboxmid_x;
        float offset_y = boxmid_y - checkboxmid_y;

        float target_length_x = label_dist + (float)(check_p->label->w + w) / 2.0f;
        float target_length_y = label_dist + (float)(check_p->label->h + h) / 2.0f;
    
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

        target_length_x = icon_dist + (float)check_p->label->w / 2.0f;
        target_length_y = icon_dist + (float)check_p->label->h / 2.0f;
    
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

    // label drawlevel hysteresis
    
    float density_mult = 100.0f;
    float level_rate = 0.0005f;

    if(labelLevel < -1.25f + density_mult * density_max) {
        labelLevel += level_rate;
    } else if (labelLevel > 0.5f + density_mult * density_max) {
        labelLevel -= level_rate;                         
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

        // x = p->cx + (int)round(p->ox);
        // y = p->cy + (int)round(p->oy);
}


void AircraftLabel::draw(SDL_Renderer *renderer) {
    //don't draw first time
    if(x == 0 || y == 0) {
        return;
    }

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

    if(w != 0) {

        SDL_Color drawColor = style.labelLineColor;

        drawColor.a = (int) (255.0f * opacity);

        //this would need to be set in view (settable label level etc)
        // if(p == selectedAircraft) {
        //     drawColor = style.selectedColor;
        // }

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

        boxRGBA(renderer, x, y, x + w, y + h, 0, 0, 0, 255);

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
   
    // if(labelLevel < 2 || p == selectedAircraft) {
    //need externally settable label level
    if(labelLevel < 2) {
        // drawSignalMarks(p, x, y);

        SDL_Color drawColor = style.labelColor;
        drawColor.a = (int) (255.0f * opacity);

        flightLabel.setFGColor(drawColor);
    	flightLabel.draw(renderer);
        // outRect = drawString(flight, x, y, mapBoldFont, drawColor); 
    	outRect = flightLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;            
    
    }

    // if(labelLevel < 1 || p == selectedAircraft) {
    if(labelLevel < 1) {
        SDL_Color drawColor = style.subLabelColor;
        drawColor.a = (int) (255.0f * opacity);

        // drawStringBG(alt, x, y + currentLine * mapFontHeight, mapFont, style.subLabelColor, style.labelBackground);   
        // outRect = drawString(alt, x, y + totalHeight, mapFont, drawColor);   
        altitudeLabel.setFGColor(drawColor);
		altitudeLabel.draw(renderer);
    	outRect = altitudeLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;                              


        // drawStringBG(speed, x, y + currentLine * mapFontHeight, mapFont, style.subLabelColor, style.labelBackground);  
        // outRect = drawString(speed, x, y + totalHeight, mapFont, drawColor);  
        speedLabel.setFGColor(drawColor);
		speedLabel.draw(renderer);
    	outRect = speedLabel.getRect();

        totalWidth = std::max(totalWidth,outRect.w);  
        totalHeight += outRect.h;      
    
    }


    //label debug
    // char debug[25] = "";
    // snprintf(debug,25,"%1.2f", p->labelLevel); 
    // drawString(debug, p->x, p->y + totalHeight, mapFont, style.red);   

    // if(maxCharCount > 1) {

    //     Sint16 vx[4] = {
    //         static_cast<Sint16>(p->cx), 
    //         static_cast<Sint16>(p->cx + (p->x - p->cx) / 2), 
    //         static_cast<Sint16>(p->x), 
    //         static_cast<Sint16>(p->x)};

    //     Sint16 vy[4] = {
    //         static_cast<Sint16>(p->cy), 
    //         static_cast<Sint16>(p->cy + (p->y - p->cy) / 2), 
    //         static_cast<Sint16>(p->y - mapFontHeight), 
    //         static_cast<Sint16>(p->y)};
        
    //     if(p->cy > p->y + currentLine * mapFontHeight) {
    //         vy[2] = p->y + currentLine * mapFontHeight + mapFontHeight;
    //         vy[3] = p->y + currentLine * mapFontHeight;
    //     } 

    //     bezierRGBA(renderer,vx,vy,4,2,style.labelLineColor.r,style.labelLineColor.g,style.labelLineColor.b,SDL_ALPHA_OPAQUE);


        // lineRGBA(renderer,p->x,p->y,p->x,p->y+currentLine*mapFontHeight,style.labelLineColor.r,style.labelLineColor.g,style.labelLineColor.b,SDL_ALPHA_OPAQUE);
    // }

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
}