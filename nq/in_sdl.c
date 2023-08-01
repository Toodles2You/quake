/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

#include <SDL2/SDL_events.h>

extern SDL_Window *window;

enum {
    PITCH_MIN = -70,
    PITCH_MAX = 80,
};

typedef enum {
    MOUSE_AVAILABLE = 1,
    MOUSE_ACTIVE = 2,
    MOUSE_RELATIVE = 4,
} mstate_t;

static mstate_t mouse_state;
static vec2_t mouse;
static vec2_t old_mouse;

static cvar_t in_mouse = {"in_mouse", "1", false};
static cvar_t m_rawinput = {"m_rawinput", "1", false};
static cvar_t m_filter = {"m_filter", "0"};

static int IN_TranslateKey(SDL_KeyboardEvent *k)
{
    int key = 0;

    switch (k->keysym.sym)
    {
    case SDLK_KP_9:
        key = K_PGUP; /* K_KP_PGUP */
        break;
    case SDLK_PAGEUP:
        key = K_PGUP;
        break;
    case SDLK_KP_3:
        key = K_PGDN; /* K_KP_PGDN */
        break;
    case SDLK_PAGEDOWN:
        key = K_PGDN;
        break;
    case SDLK_KP_7:
        key = K_HOME; /* K_KP_HOME */
        break;
    case SDLK_HOME:
        key = K_HOME;
        break;
    case SDLK_KP_1:
        key = K_END; /* K_KP_END */
        break;
    case SDLK_END:
        key = K_END;
        break;
    case SDLK_KP_4:
        key = K_LEFTARROW; /* K_KP_LEFTARROW */
        break;
    case SDLK_LEFT:
        key = K_LEFTARROW;
        break;
    case SDLK_KP_6:
        key = K_RIGHTARROW; /* K_KP_RIGHTARROW */
        break;
    case SDLK_RIGHT:
        key = K_RIGHTARROW;
        break;
    case SDLK_KP_2:
        key = K_DOWNARROW; /* K_KP_DOWNARROW */
        break;
    case SDLK_DOWN:
        key = K_DOWNARROW;
        break;
    case SDLK_KP_8:
        key = K_UPARROW; /* K_KP_UPARROW */
        break;
    case SDLK_UP:
        key = K_UPARROW;
        break;
    case SDLK_ESCAPE:
        key = K_ESCAPE;
        break;
    case SDLK_KP_ENTER:
        key = K_ENTER; /* K_KP_ENTER */
        break;
    case SDLK_RETURN:
        key = K_ENTER;
        break;
    case SDLK_TAB:
        key = K_TAB;
        break;
    case SDLK_F1:
        key = K_F1;
        break;
    case SDLK_F2:
        key = K_F2;
        break;
    case SDLK_F3:
        key = K_F3;
        break;
    case SDLK_F4:
        key = K_F4;
        break;
    case SDLK_F5:
        key = K_F5;
        break;
    case SDLK_F6:
        key = K_F6;
        break;
    case SDLK_F7:
        key = K_F7;
        break;
    case SDLK_F8:
        key = K_F8;
        break;
    case SDLK_F9:
        key = K_F9;
        break;
    case SDLK_F10:
        key = K_F10;
        break;
    case SDLK_F11:
        key = K_F11;
        break;
    case SDLK_F12:
        key = K_F12;
        break;
    case SDLK_BACKSPACE:
        key = K_BACKSPACE;
        break;
    case SDLK_KP_DECIMAL:
        key = K_DEL; /* K_KP_DEL */
        break;
    case SDLK_DELETE:
        key = K_DEL;
        break;
    case SDLK_PAUSE:
        key = K_PAUSE;
        break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        key = K_SHIFT;
        break;
    case SDLK_EXECUTE:
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        key = K_CTRL;
        break;
    case SDLK_LALT:
    case SDLK_LGUI:
    case SDLK_RALT:
    case SDLK_RGUI:
        key = K_ALT;
        break;
    case SDLK_KP_5:
        key = '5'; /* K_KP_5 */
        break;
    case SDLK_INSERT:
        key = K_INS;
        break;
    case SDLK_KP_0:
        key = K_INS; /* K_KP_INS */
        break;
    case SDLK_KP_MULTIPLY:
        key = '*';
        break;
    case SDLK_KP_PLUS:
        key = '+'; /* K_KP_PLUS */
        break;
    case SDLK_KP_MINUS:
        key = '-'; /* K_KP_MINUS */
        break;
    case SDLK_KP_DIVIDE:
        key = '/'; /* K_KP_SLASH */
        break;
    case SDLK_SPACE:
        key = K_SPACE;
        break;
    default:
        key = *SDL_GetKeyName(k->keysym.sym);
        if (key >= 'A' && key <= 'Z')
            key = key - 'A' + 'a';
        break;
    }
    return key;
}

static int IN_TranslateButton(SDL_MouseButtonEvent *b)
{
    switch (b->button)
    {
    case SDL_BUTTON_LEFT:
        return K_MOUSE1;
    case SDL_BUTTON_RIGHT:
        return K_MOUSE2;
    case SDL_BUTTON_MIDDLE:
        return K_MOUSE3;
    }
    return -1;
}

static void IN_HandleEvent(SDL_Event* event, qboolean* warp_mouse)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        Key_Event(IN_TranslateKey(&event->key), event->type == SDL_KEYDOWN);
        break;
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
        int button = IN_TranslateButton(&event->button);

        if (button == -1)
            break;

        Key_Event(button, event->type == SDL_MOUSEBUTTONDOWN);
        break;
    }
    case SDL_MOUSEMOTION:
    {
        if (!(mouse_state & MOUSE_ACTIVE))
            break;

        mouse[0] += event->motion.xrel * 2;
        mouse[1] += event->motion.yrel * 2;

        if (!(mouse_state & MOUSE_RELATIVE) && (mouse[0] || mouse[1]))
        {
            *warp_mouse = true;
        }

        break;
    }
    case SDL_MOUSEWHEEL:
    {
        int dir = (event->wheel.y < 0);

        if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
        {
            dir = !dir;
        }
        
        Key_Event(K_MWHEELUP + dir, true);
        Key_Event(K_MWHEELUP + dir, false);
        break;
    }
    }
}

void Sys_SendKeyEvents()
{
    SDL_Event event;
    qboolean warp_mouse = false;

    while (SDL_PollEvent(&event))
    {
        IN_HandleEvent(&event, &warp_mouse);
    }

    if (warp_mouse && window)
    {
        /* move the mouse to the window center again */
        SDL_WarpMouseInWindow(window, vid.width / 2, vid.height / 2);
    }
}

static void Force_CenterView_f()
{
    cl.viewangles[PITCH] = 0;
}

void IN_Init()
{
    mouse_state = MOUSE_AVAILABLE;
    mouse[0] = mouse[1] = 0;
    
    Cvar_RegisterVariable(&in_mouse);
    Cvar_RegisterVariable(&m_rawinput);
    Cvar_RegisterVariable(&m_filter);
    Cmd_AddCommand("force_centerview", Force_CenterView_f);
}

void IN_Shutdown()
{
    mouse_state = 0;
}

static void IN_ActivateGrabs()
{
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetWindowMouseGrab(window, SDL_TRUE);

	if (m_rawinput.value && SDL_SetRelativeMouseMode(SDL_TRUE) == 0)
	{
        mouse_state |= MOUSE_RELATIVE;
	}
	else
	{
		SDL_WarpMouseInWindow(window, vid.width / 2, vid.height / 2);
        mouse_state &= ~MOUSE_RELATIVE;
	}

	SDL_SetWindowKeyboardGrab(window, SDL_TRUE);

    mouse_state |= MOUSE_ACTIVE;
    mouse[0] = mouse[1] = 0;
}

static void IN_DeactivateGrabs()
{
	SDL_SetWindowKeyboardGrab(window, SDL_FALSE);

	if (mouse_state & MOUSE_RELATIVE)
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	SDL_SetWindowMouseGrab(window, SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

    mouse_state &= ~(MOUSE_ACTIVE | MOUSE_RELATIVE);
}

void IN_Commands()
{
    if (!window)
        return;

    if (!(mouse_state & MOUSE_AVAILABLE))
        return;

    if (key_dest == key_game)
    {
        if (!(mouse_state & MOUSE_ACTIVE))
            IN_ActivateGrabs();
    }
    else
    {
        if (mouse_state & MOUSE_ACTIVE)
            IN_DeactivateGrabs();
    }
}

void IN_Move(usercmd_t *cmd)
{
    if (!(mouse_state & MOUSE_ACTIVE))
        return;

    if (m_filter.value)
    {
        mouse[0] = (mouse[0] + old_mouse[0]) * 0.5;
        mouse[1] = (mouse[1] + old_mouse[1]) * 0.5;
    }

    old_mouse[0] = mouse[0];
    old_mouse[1] = mouse[1];

    mouse[0] *= sensitivity.value;
    mouse[1] *= sensitivity.value;

    qboolean looking = (in_mlook.state & 1);
    qboolean strafing = (in_strafe.state & 1);

    // add mouse X/Y movement to cmd
    if (strafing || (lookstrafe.value && looking))
    {
        cmd->sidemove += m_side.value * mouse[0];
    }
    else
    {
        cl.viewangles[YAW] -= m_yaw.value * mouse[0];
    }

    if (looking)
        V_StopPitchDrift();

    if (looking && !strafing)
    {
        cl.viewangles[PITCH] += m_pitch.value * mouse[1];
        cl.viewangles[PITCH] = SDL_clamp(cl.viewangles[PITCH], PITCH_MIN, PITCH_MAX);
    }
    else
    {
        if (strafing && noclip_anglehack)
        {
            cmd->upmove -= m_forward.value * mouse[1];
        }
        else
        {
            cmd->forwardmove -= m_forward.value * mouse[1];
        }
    }

    mouse[0] = mouse[1] = 0;
}
