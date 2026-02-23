
#ifndef _VID_H
#define _VID_H

#define VID_CBITS 6
#define VID_GRADES (1 << VID_CBITS)

// a pixel can be one, two, or four bytes
typedef byte pixel_t;

typedef struct vrect_s
{
	int x, y, width, height;
	struct vrect_s *pnext;
} vrect_t;

typedef struct
{
	unsigned int width;
	unsigned int height;
	float aspect; // width / height -- < 0 is taller than wide
	bool recalc_refdef; // if true, recalc vid-based stuff
	unsigned int conwidth;
	unsigned int conheight;
} viddef_t;

extern viddef_t vid; // global video state
extern unsigned int d_8to24table[256];
extern void (*vid_menudrawfn) ();
extern void (*vid_menukeyfn) (int key);

void VID_Init (char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void VID_Shutdown (void);
// Called at shutdown

#endif /* !_VID_H */
