
#include <stdio.h>
#include <malloc.h>
#include <png.h>
#include "include/pngbits.h"


void setByte(unsigned char b, int mask, int pos){
}


int readpng_init(FILE *infile, png *pngdata)
{
    unsigned char sig[8];
    long width,height;
    int  bit_depth, color_type;
 


    /* first do a quick check that the file really is a PNG image; could
     * have used slightly more general png_sig_cmp() function instead */

    fread(sig, 1, 8, infile);
    if (!png_check_sig(sig, 8))
        return 1;   /* bad signature */


    /* could pass pointers to user-defined error handlers instead of NULLs: */

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        return 4;   /* out of memory */

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 4;   /* out of memory */
    }


    /* we could create a second info struct here (end_info), but it's only
     * useful if we want to keep pre- and post-IDAT chunk info separated
     * (mainly for PNG-aware image editors and converters) */


    /* setjmp() must be called in every function that calls a PNG-reading
     * libpng function */

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 2;
    }


    png_init_io(png_ptr, infile);
    png_set_sig_bytes(png_ptr, 8);  /* we already read the 8 signature bytes */

    png_read_info(png_ptr, info_ptr);  /* read all PNG info up to image data */


    /* alternatively, could make separate calls to png_get_image_width(),
     * etc., but want bit_depth and color_type for later [don't care about
     * compression_type and filter_type => NULLs] */

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
      NULL, NULL, NULL);
//    *pWidth = width;
//    *pHeight = height;
	pngdata->width = width;
	pngdata->height = height;
	pngdata->bitdepth = bit_depth;
	pngdata->colortype = color_type;
	pngdata->png_ptr = png_ptr;
	pngdata->info_ptr = info_ptr;

    /* OK, that's all we need for now; return happy */

    return 0;
}


unsigned char *readpng_get_image( png* pngdata)
{
    double  gamma;
    png_uint_32  i, rowbytes;
    png_bytepp  row_pointers = NULL;
    png_structp png_ptr = pngdata->png_ptr;
    png_infop info_ptr = pngdata->info_ptr;
    int color_type = pngdata->colortype;
    int bit_depth = pngdata->bitdepth;
    int height = pngdata->height;
	double display_exponent = DISPLAY_EXPONENT;
    int *pChannels = &(pngdata->channels);
    unsigned long *pRowbytes = &(pngdata->rowbytes);
    unsigned char * image_data;
    /* setjmp() must be called in every function that calls a PNG-reading
     * libpng function */

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    /* expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
     * transparency chunks to full alpha channel; strip 16-bit-per-sample
     * images to 8 bits per sample; and convert grayscale to RGB[A] */

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);


    /* unlike the example in the libpng documentation, we have *no* idea where
     * this file may have come from--so if it doesn't have a file gamma, don't
     * do any correction ("do no harm") */

    if (png_get_gAMA(png_ptr, info_ptr, &gamma))
        png_set_gamma(png_ptr, display_exponent, gamma);


    /* all transformations have been registered; now update info_ptr data,
     * get rowbytes and channels, and allocate image memory */

    png_read_update_info(png_ptr, info_ptr);

    *pRowbytes = rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    *pChannels = (int)png_get_channels(png_ptr, info_ptr);

    if ((image_data = (unsigned char *)malloc(rowbytes*height)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }
    if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(image_data);
        image_data = NULL;
        return NULL;
    }

    /* set the individual row_pointers to point at the correct offsets */

    for (i = 0;  i < height;  ++i)
        row_pointers[i] = image_data + i*rowbytes;

    /* now we can go ahead and just read the whole image */

    png_read_image(png_ptr, row_pointers);


    /* and we're done!  (png_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired) */

    free(row_pointers);
    row_pointers = NULL;

    png_read_end(png_ptr, NULL);

    pngdata->image_data = image_data;

    return image_data;
}

unsigned int rgba_pixel(int x, int y,png *pngdata){
	long height = pngdata->height;
	long width = pngdata->width;
	unsigned char *data = (unsigned char *) pngdata->image_data;
	unsigned int * res = (unsigned int*)&(data[(width*y+x)*pngdata->bytes_per_pix]);

	return *res;
}

FILE * readpng_or_exit(char *filename, png *pngdata){
	FILE *f = fopen(filename,"r");

	int ret = readpng_init(f,pngdata);
	if(ret != 0){
		switch(ret){
			case 1:	
				perror("Invalid PNG file.\n");
			break;
			case 4:	
				perror("Out of memory.\n");
			break;
			default:
				perror("Error, could not load image.\n");
				
		}	
		exit(1);
	}

	printf("Width: %ld\n", pngdata->width);
	printf("Height: %ld\n", pngdata->height);
	printf("Bitdepth: %d\n", pngdata->bitdepth);
	printf("Color Type: %d\n", pngdata->colortype);

	if(pngdata->bitdepth != 8){
		fprintf(stderr,"Error: bit depth must be 8, is %d.\n",pngdata->bitdepth);
		exit(1);
	}
	if((pngdata->colortype != RGB) && (pngdata->colortype != RGBA)){
		fprintf(stderr,"Error: color type must be 2 (RGB) or 6 (RGBA), instead is %d.\n",pngdata->colortype);
		exit(1);
	}
	switch(pngdata->colortype ){
		case RGB:
			pngdata->bytes_per_pix = 3;
		break;
		case RGBA:
			pngdata->bytes_per_pix = 4;
		break;
	}
	
        unsigned char *imgdata = readpng_get_image(pngdata);
	if(imgdata == NULL){
		perror("Invalid image.\n");
		exit(1);
	}

	unsigned int avgr,avgg,avgb,avga;
	avgr=avgg=avgb=avga = 0;
	int cntx,cnty;
	for(cntx = 0; cntx< pngdata->width;cntx++)
		for(cnty = 0; cnty< pngdata->height;cnty++)
		{
			unsigned int pix = rgba_pixel(cntx,cnty,pngdata);
			avgr += (unsigned int)(pix & 0xff);
			avgg += (unsigned int)((pix >> 8) & 0xff);
			avgb += (unsigned int)((pix >> 16) & 0xff);
			avga += (unsigned int)((pix >> 24) & 0xff);
//			printf("%08x\n",pix);
//			printf("RGBA: %02x %02x %02x %02x\n", (pix & 0xff),((pix >> 8) & 0xff),( (pix >> 16) & 0xff), ((pix >> 24) & 0xff));
//			printf("RGBA: %3d,%3d,%3d,%3d\n", (pix & 0xff),((pix >> 8) & 0xff),( (pix >> 16) & 0xff), ((pix >> 24) & 0xff));
		}

	long tot = pngdata->width * pngdata->height;


	printf("R %ld\n", avgr/tot);
	printf("G %ld\n", avgg/tot);
	printf("B %ld\n", avgb/tot);
	printf("A %ld\n", avga/tot);

	return f;
}



int main(int argc, char *argv[]){
	png pngdata;
	
	if(argc != 2){
		perror("Use PNG filename in command line");
		return;
	}
	char *filename = argv[1];	
	
	printf("Opening file: %s\n", filename);

	FILE *f = readpng_or_exit(filename,&pngdata);	
	
	fclose(f);

}
