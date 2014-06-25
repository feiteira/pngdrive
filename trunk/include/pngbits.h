#include <png.h>
/*
http://www.w3.org/TR/PNG-Chunks.html
http://webcache.googleusercontent.com/search?q=cache:http://128.243.106.145:8000/src/libpng/png.h

*/
#define RGB 2
#define RGBA 6

typedef struct png_store{
	long width;
	long height;
	int bitdepth;
	int colortype;
	int pixelsize;//in bytes
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *image_data;
	int channels; 
	unsigned long rowbytes;
	png_bytepp  row_pointers ;

	// data
	unsigned char *drivedata;
	int drivesize;
	char *key;
	unsigned int mask;
	
} png_store;

#define DISPLAY_EXPONENT 2.2f

extern unsigned int mask;
extern char* key;

unsigned int mask;
char* key;


FILE * readpng_or_exit(char *filename, png_store *pngdata);
void loadPNGDriveData(png_store *pngdata);
void savePNGDriveData(png_store *pngdata);
int getPNGDriveSize(png_store *pngdata);
int writepng(char* filename, png_store *pngdata);


