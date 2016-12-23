import pygame
import os
from time import sleep
import RPi.GPIO as GPIO
 
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

sql_str = list("0x");

while True:

    for event in pygame.event.get():
        if event.type == pygame.KEYDOWN:
		
		print pygame.key.name(event.key)

		if event.key == pygame.K_x:
			dial += 1
		elif event.key == pygame.K_z:
			dial -= 1
		elif event.key == pygame.K_m:
			dialColor = (127,255,255)
		elif event.key == pygame.K_n:
			dialColor = (255,255,255)
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
			sql_str = list("0x")

		lcd.fill((0,0,0))
	        text_surface = font_big.render('%d'%dial, True, dialColor)
            	rect = text_surface.get_rect(center=(160,100))
            	lcd.blit(text_surface, rect)
		sql_surface = font_big.render('%d'%sql, True, WHITE)
		rect = text_surface.get_rect(center=(160,140))
		lcd.blit(sql_surface, rect)
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

