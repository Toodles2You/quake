
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
	pixel_t *buffer;			// invisible buffer
	pixel_t *colormap;			// 256 * VID_GRADES size
	unsigned short *colormap16; // 256 * VID_GRADES size
	int fullbright;				// index of first fullbright color
	unsigned rowbytes;			// may be > width if displayed in a window
	unsigned width;
	unsigned height;
	float aspect; // width / height -- < 0 is taller than wide
	int numpages;
	int recalc_refdef; // if true, recalc vid-based stuff
	pixel_t *conbuffer;
	int conrowbytes;
	unsigned conwidth;
	unsigned conheight;
	int maxwarpwidth;
	int maxwarpheight;
	pixel_t *direct; // direct drawing to framebuffer, if not
					 //  NULL
} viddef_t;

extern viddef_t vid; // global video state
extern unsigned short d_8to16table[256];
extern unsigned d_8to24table[256];
extern void (*vid_menudrawfn)();
extern void (*vid_menukeyfn)(int key);

void VID_SetPalette(unsigned char *palette);
// called at startup and after any gamma correction

void VID_ShiftPalette(unsigned char *palette);
// called for bonus and pain flashes, and for underwater color changes

void VID_Init(unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void VID_Shutdown();
// Called at shutdown

#endif /* !_VID_H */
