extern "C" {
	#include "hrenderer.h"
}

bool isGameController(int joystick_index) {
	return SDL_IsGameController(joystick_index);
}

const char* getStringForButton(Uint8 button) {
	return SDL_GameControllerGetStringForButton((SDL_GameControllerButton)button);	
}

