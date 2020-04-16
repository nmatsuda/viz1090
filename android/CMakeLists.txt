cmake_minimum_required(VERSION 3.4.1)

message(${CMAKE_SOURCE_DIR})
string(TOLOWER ${CMAKE_BUILD_TYPE} ANDROID_BUILD_DIR)
set(DISTRIBUTION_DIR ${CMAKE_SOURCE_DIR}/distribution/android/SDL2/intermediates/ndkBuild)
set(SOURCE_FILES ../map1090.cpp ../AppData.cpp ../AircraftList.cpp ../Aircraft.cpp ../anet.c ../interactive.c ../mode_ac.c ../mode_s.c ../net_io.c ../Input.cpp ../View.cpp ../Map.cpp ../parula.c ../monokai.c)

set(SDL_LOCATION ${CMAKE_SOURCE_DIR}/external/SDL2)

add_library( SDL2 SHARED IMPORTED )
add_library( SDL2_ttf SHARED IMPORTED )
add_library( SDL2_gfx SHARED IMPORTED )

set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION
${DISTRIBUTION_DIR}/${ANDROID_BUILD_DIR}/obj/local/${ANDROID_ABI}/libSDL2.so)
set_target_properties(SDL2_ttf PROPERTIES IMPORTED_LOCATION
${DISTRIBUTION_DIR}/${ANDROID_BUILD_DIR}/obj/local/${ANDROID_ABI}/libSDL2_ttf.so)
set_target_properties(SDL2_gfx PROPERTIES IMPORTED_LOCATION
${DISTRIBUTION_DIR}/${ANDROID_BUILD_DIR}/obj/local/${ANDROID_ABI}/libSDL2_gfx.so)

include_directories(${SDL_LOCATION}/SDL2/include)
include_directories(${SDL_LOCATION}/SDL2_ttf)
include_directories(${SDL_LOCATION}/SDL2_gfx)


add_library( main SHARED ${SDL_LOCATION}/SDL2/src/main/android/SDL_android_main.c ${SOURCE_FILES} )

target_link_libraries( main SDL2 SDL2_ttf SDL2_gfx log)
