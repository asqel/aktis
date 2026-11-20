#include "client.h"

void on_key_start(SDL_Event *ev) {
	if (ev->type != SDL_EVENT_KEY_DOWN)
		return ;
	
	if (ev->key.key == SDLK_RETURN) {
		if (!clt.name[0])
			return ;
		clt.state = CLT_LOADING;
		clt.sock.on_connect = clt_on_connect;
		clt.sock.on_disconnect = clt_on_disconnect;
		clt.sock.on_recv = clt_on_recv;
		clt.sock.on_send_ready = clt_on_send_ready;
		clt.has_sock = 1;
		if (ra_client_init(&clt.sock, "asqel.ddns.net", 42124, RA_FLAG_NODELAY)) {
			client_fail("pas trouve le server");
			clt.has_sock = 0;
		}
		return ;
	}
	SDL_Keycode key = ev->key.key;

	int len = strlen(clt.name);
	if (SDLK_A <= key && key <= SDLK_Z) {
		if (len == 6)
			return ;
		clt.name[len] = key - SDLK_A + 'A';
	}
	else if (key == SDLK_BACKSPACE && len)
		clt.name[len - 1] = '\0';
}

void on_key_ingame(SDL_Event *ev) {
	static uint8_t pressed_keys[32] = {0};

	if (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP) {
		int key = -1;
		if (ev->key.scancode == SDL_SCANCODE_W)
			key = 0;
		else if (ev->key.scancode == SDL_SCANCODE_D)
			key = 1;
		else if (ev->key.scancode == SDL_SCANCODE_S)
			key = 2;
		else if (ev->key.scancode == SDL_SCANCODE_A)
			key = 3;
		else if (ev->key.scancode == SDL_SCANCODE_I)
			key = 4;
		else if (ev->key.scancode == SDL_SCANCODE_L)
			key = 5;
		else if (ev->key.scancode == SDL_SCANCODE_K)
			key = 6;
		else if (ev->key.scancode == SDL_SCANCODE_J)
			key = 7;

		if (key == -1)
			return ;
		uint8_t state = (ev->type == SDL_EVENT_KEY_DOWN);
		if (pressed_keys[key] != state) {
			packet_append(&clt.send, OPC_SET_KEY, "bb", ev->type == SDL_EVENT_KEY_DOWN, key);
		}
		pressed_keys[key] = state;
	}
}

void clt_on_key(SDL_Event *ev) {
	switch (clt.state) {
		case CLT_INGAME:
			on_key_ingame(ev);
			break;
		case CLT_START:
			on_key_start(ev);
			break;
	}
}
