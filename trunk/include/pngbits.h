#include <png.h>

#define RGB 2
#define RGBA 6

typedef struct png{
	long width;
	long height;
	int bitdepth;
	int colortype;
	int bytes_per_pix;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *image_data;
	int channels; 
	unsigned long rowbytes;
} png;

#define DISPLAY_EXPONENT 2.2f
