#!/usr/bin/python

import pygame
import os
from time import sleep

#Colours
WHITE = (255,255,255)
 
#os.putenv('SDL_FBDEV', '/dev/fb1')
pygame.init()
pygame.mouse.set_visible(False)
lcd = pygame.display.set_mode((320, 240))

font_big = pygame.font.Font(None, 100)
font_small = pygame.font.Font(None, 18)

lcd.fill((0,0,0))
pygame.display.update()
 
top_1 = (0,0,80,40);
top_2 = (80,0,80,40);
top_3 = (160,0,80,40);
top_4 = (240,0,80,40);

pygame.draw.rect(lcd, (100,100,100), top_1, 2)
pygame.draw.rect(lcd, (100,100,100), top_2, 2)
pygame.draw.rect(lcd, (100,100,100), top_3, 2)
pygame.draw.rect(lcd, (100,100,100), top_4, 2)

text_surface = font_small.render('DEMOD', True, WHITE)
rect = text_surface.get_rect(center=(40, 20))
lcd.blit(text_surface, rect)

text_surface = font_small.render('SQLCH', True, WHITE)
rect = text_surface.get_rect(center=(120, 20))
lcd.blit(text_surface, rect)

text_surface = font_small.render('GAIN', True, WHITE)
rect = text_surface.get_rect(center=(200, 20))
lcd.blit(text_surface, rect)

text_surface = font_small.render('MENU', True, WHITE)
rect = text_surface.get_rect(center=(280, 20))
lcd.blit(text_surface, rect)


sql_gain_box = (0,40,32,200)

pygame.draw.rect(lcd, (10,10,10), sql_gain_box, 2)

for x in range(0,6):
	pygame.draw.rect(lcd, (10,10,10), (32 + x * 48,40,48,80),2)
 	text_surface = font_big.render('%d'%x, True, WHITE)
        rect = text_surface.get_rect(center=(32 + (x + .5) * 48, 80))
        lcd.blit(text_surface, rect)

while True:
	pygame.display.update()
	sleep(1)
