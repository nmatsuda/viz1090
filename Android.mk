LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main.out

SDL_PATH := ~/Downloads/SDL

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(SDL_PATH)/include 	\
	$(LOCAL_PATH)/$(SDL_PATH) 			
	
#LOCALCFLAGS= 

LOCAL_SRC_FILES := \
	$(SDL_PATH)/src/main/android/SDL_android_main.cpp \
	allstates.c \
	input.c \
	mapdata.c \
	monokai.c \
	planeObj.c \
	anet.c \
	font.c \
	interactive.c \
	mode_ac.c \
	net_io.c \
	status.c \
	draw.c \
	init.c \
	list.c \
	mode_s.c \
	parula.c \
	view1090.c

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_gfx SDL2_ttf

LOCAL_LDLIBS := -lGLESv1_CM #-lstdc++

include $(BUILD_EXECUTABLE)