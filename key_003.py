#!/usr/bin/python

import pygame
import os
from time import sleep
import RPi.GPIO as GPIO
from rtl_fm_python_thread import *

make_rtl_fm_thread(block=False)

#Note #21 changed to #27 for rev2 Pi
button_map = {23:(255,0,0), 22:(0,255,0), 27:(0,0,255), 17:(0,0,0)}
 
#Setup the GPIOs as inputs with Pull Ups since the buttons are connected to GND
GPIO.setmode(GPIO.BCM)
for k in button_map.keys():
    GPIO.setup(k, GPIO.IN, pull_up_down=GPIO.PUD_UP)
 
#Colours
WHITE = (255,255,255)
 
os.putenv('SDL_FBDEV', '/dev/fb1')
pygame.init()
pygame.mouse.set_visible(False)
lcd = pygame.display.set_mode((320, 240))
lcd.fill((0,0,0))
pygame.display.update()
 
font_big = pygame.font.Font(None, 100)

dial = 0

dialColor = (255,255,255)

sql = 0

sql_str = list("0x")

freq = [4,4,2,2,7,5]

freq_idx = 0;

wasdown = 0;

while True:

    for event in pygame.event.get():
        if event.type == pygame.KEYDOWN:
		if event.key == pygame.K_x:
			freq[freq_idx] = (freq[freq_idx] + 1) % 10            
            		set_frequency(int(''.join(str(x) for x in freq) + "000"))
		elif event.key == pygame.K_z:
			freq[freq_idx] = (freq[freq_idx] - 1) % 10
            		set_frequency(int(''.join(str(x) for x in freq) + "000"))            
		elif event.key == pygame.K_m:
			wasdown = 1;
		elif event.key == pygame.K_n:
			if(wasdown == 1):
				wasdown = 0
				freq_idx = (freq_idx + 1) % 6
                elif event.key == pygame.K_0:
                        sql_str.append('0')
                elif event.key == pygame.K_1:
                        sql_str.append('1')
                elif event.key == pygame.K_2:
                        sql_str.append('2')
                elif event.key == pygame.K_3:
                        sql_str.append('3')
                elif event.key == pygame.K_4:
                        sql_str.append('4')
                elif event.key == pygame.K_5:
                        sql_str.append('5')
                elif event.key == pygame.K_6:
                        sql_str.append('6')
                elif event.key == pygame.K_7:
                        sql_str.append('7')
                elif event.key == pygame.K_8:
                        sql_str.append('8')
                elif event.key == pygame.K_9:
                        sql_str.append('9')
                elif event.key == pygame.K_a:
                        sql_str.append('a')
                elif event.key == pygame.K_b:
                        sql_str.append('b')
                elif event.key == pygame.K_c:
                        sql_str.append('c')
                elif event.key == pygame.K_d:
                        sql_str.append('d')
                elif event.key == pygame.K_e:
                        sql_str.append('e')
                elif event.key == pygame.K_f:
                        sql_str.append('f')
		elif event.key == pygame.K_RETURN:
			sql = int("".join(sql_str), 0)
            		set_squelch(sql)
			sql_str = list("0x")

		lcd.fill((0,0,0))
	        #text_surface = font_big.render('%d'%dial, True, dialColor)
            	#rect = text_surface.get_rect(center=(160,100))
            	#lcd.blit(text_surface, rect)

		for x in range(0,6):
			text_surface = font_big.render('%d'%freq[x], True, WHITE)
			rect = text_surface.get_rect(center=((x + .5) * 320 / 6, 80))
			lcd.blit(text_surface, rect)

		pygame.draw.rect(lcd, WHITE, (freq_idx * 320 / 6, 50, 320 / 6, 70), 1)

		pygame.draw.rect(lcd, (255, 200, 10), (10,10,20, 220 * sql / 1024), 0)
		#sql_surface = font_big.render('%d'%sql, True, WHITE)
		#rect = text_surface.get_rect(center=(160,140))
		#lcd.blit(sql_surface, rect)
	    	pygame.display.update()

    # Scan the buttons
#    for (k,v) in button_map.items():
#        if GPIO.input(k) == False:
#            lcd.fill(v)
#            text_surface = font_big.render('%d'%k, True, WHITE)
#            rect = text_surface.get_rect(center=(160,120))
#            lcd.blit(text_surface, rect)
#            pygame.display.update()
#    sleep(0.1)
