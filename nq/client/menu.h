
#ifndef _MENU_H
#define _MENU_H

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
enum
{
    MNET_IPX = 1,
    MNET_TCP,
};

extern int m_activenet;

//
// menus
//
void M_Init();
void M_Keydown(int key);
void M_Draw();
void M_ToggleMenu_f();

#endif /* !_MENU_H */
