#include <png.h>


typedef struct png{
	long width;
	long height;
	int bitdepth;
	int colortype;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *image_data;
	int *pChannels; 
	unsigned long *pRowbytes;
} png;

#define DISPLAY_EXPONENT 2.2f
