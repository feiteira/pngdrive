#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <png.h>
#include <openssl/md5.h>
#include "include/pngdrive.h"
#include "include/bitmasks.h"
#include "include/pngbits.h"


int writepng(char* filename, png_store *pngdata)
{
	int code = 0;
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row;
	int width = pngdata->width;
	int height = pngdata->height;
//	float *buffer = pngdata->image_data;
	
	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		code = 1;
		exit(1);
	}

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		code = 1;
		exit(1);
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 1;
		exit(1);
	}

	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		code = 1;
		exit(1);
	}

	png_init_io(png_ptr, fp);



	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height,
			pngdata->bitdepth, pngdata->colortype, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	png_write_image(png_ptr,pngdata->row_pointers);	
	// End write
	png_write_end(png_ptr, NULL);
/*
	finalise:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);
*/
	return code;
}

int readpng_init(FILE *infile, png_store *pngdata)
{
    unsigned char sig[8];
    long width,height;
    int  bit_depth, color_type;
 

    /* first do a quick check that the file really is a PNG image; could
     * have used slightly more general png_sig_cmp() function instead */

    size_t fr = fread(sig, 1, 8, infile);
    if(fr != 1*8)
        return 1;   /* bad signature */

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
	pngdata->width = width;
	pngdata->height = height;
	pngdata->bitdepth = bit_depth;
	pngdata->colortype = color_type;
	pngdata->png_ptr = png_ptr;
	pngdata->info_ptr = info_ptr;

    /* OK, that's all we need for now; return happy */

    return 0;
}


unsigned char *readpng_get_image( png_store* pngdata)
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

//    free(row_pointers);
//    row_pointers = NULL;

    png_read_end(png_ptr, NULL);

    pngdata->image_data = image_data;
    pngdata->row_pointers =  row_pointers;

    return image_data;
}

inline void push_bit_to_ram(char bitset,unsigned char *data, unsigned long pos){
	unsigned char * ptr = &(data[pos/8]);	
	int bpos = pos % 8;
	int bit = 1 << bpos;

	if(bitset)//sets the bit
		*ptr |= bit;
	else//unsets the bit
		*ptr &= (0xFF ^ bit);
}

inline void pull_bit_from_ram(unsigned long pos, png_store * pngdata, unsigned int *masks){
	unsigned char * data = pngdata->drivedata;
	unsigned char * ptr = &(data[pos/8]);	
	unsigned char bit = ((*ptr) >> (pos % 8)) & 1;
	unsigned char pixelsize = pngdata->pixelsize;

	int bits_per_pixel = numberOfSetBits(pngdata->mask);
	unsigned long pixel_pos = pos / bits_per_pixel;// this will give the correct pixel

	unsigned int *pix = (unsigned int *) &(pngdata->image_data[pixel_pos*pixelsize]);
	
//	*pix = 0xff;	
	if(bit)
		*pix |= masks[pos % bits_per_pixel];
	else
		*pix &= (0xFFFFFFFF ^ masks[pos % bits_per_pixel]);
	
}

inline unsigned int rgba_pixel(int x, int y,png_store *pngdata){
	long height = pngdata->height;
	long width = pngdata->width;
	unsigned char *data = (unsigned char *) pngdata->image_data;
	unsigned int * res = (unsigned int*)&(data[(height*x+y)*pngdata->pixelsize]);

	return *res;
}

void compute_md5(char *str, unsigned char digest[16]) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str, strlen(str));
    MD5_Final(digest, &ctx);
}

void md5toString(unsigned char digest[16], char * str){
	int cnt  = 0;
	char *iter = str;
	for(; cnt < 16; cnt ++, iter+=2)
		sprintf(iter,"%02x",digest[cnt]);
}

void xor_drive(png_store *pngdata){
	if(pngdata->key == NULL)
		return;

	unsigned char digest[16];
	char * tmp = malloc(strlen(pngdata->key)+8);
	char hexstr[33];
	
	int cnt = 0;
	for(; cnt < pngdata->drivesize; cnt++){
		if(cnt %16 == 0){
			sprintf(tmp,"%s%08x",pngdata->key,cnt/16);
			compute_md5(tmp,digest);
			md5toString(digest,hexstr);
//			printf("%s => %s\n",tmp,hexstr);
		}
		pngdata->drivedata[cnt] ^= digest[cnt%16];
	}
	free(tmp);
}

void savePNGDriveData(png_store *pngdata){
	unsigned long cnt = 0;
	unsigned int * masks = bitmasks(pngdata->mask);	

	xor_drive(pngdata);// encrypt when saving

	for(cnt = 0; cnt < pngdata->drivesize * 8; cnt++){
		pull_bit_from_ram(cnt, pngdata,masks);
	}
	free(masks);
}

void loadPNGDriveData(png_store *pngdata){
	int mask =  pngdata->mask;
	int size = getPNGDriveSize(pngdata);
	pngdata->drivedata = (unsigned char *) malloc(size);
	unsigned char *drivedata = pngdata->drivedata;
	unsigned long bitcount = 0;
	unsigned int * masks = bitmasks(mask);	
	
	int cntx,cnty;
	unsigned long bitcounter = 0;
	unsigned int *maskptr;
	char bitset;
	unsigned int pix;
	for(cntx = 0; cntx< pngdata->width;cntx++)
		for(cnty = 0; cnty< pngdata->height;cnty++){
			for(maskptr = masks; *maskptr != 0; maskptr++){
				bitset = 0;
				pix = rgba_pixel(cntx,cnty,pngdata);
				if(*maskptr & pix)
					bitset = 1;
				push_bit_to_ram(bitset,drivedata,bitcounter);
				bitcounter ++;
			}
		}

	xor_drive(pngdata); // decript after loading

	printf("loaded %ld bytes.\n",bitcounter/8);
	free(masks);
}



FILE * readpng_or_exit(char *filename, png_store *pngdata){
	FILE *f = fopen(filename,"r");

	if(f == NULL){
		DEBUG fprintf(stderr,"Error, could not open file: %s\n", filename);
		exit(1);
	}
	

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
			pngdata->pixelsize = 3;
		break;
		case RGBA:
			pngdata->pixelsize = 4;
		break;
	}
	
        unsigned char *imgdata = readpng_get_image(pngdata);
	if(imgdata == NULL){
		perror("Invalid image.\n");
		exit(1);
	}
/*
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
*/


	return f;
}



// drive size in bytes
int getPNGDriveSize(png_store *pngdata){
	int bits_per_pix = numberOfSetBits(pngdata->mask);	
	pngdata->drivesize= (pngdata->width*pngdata->height*bits_per_pix)/8;
	return pngdata->drivesize;
}
/*
int main(int argc, char *argv[]){
	png_store pngdata;
	pngdata.key=NULL;
	pngdata.mask = DEFAULT_MASK;
	pngdata.mask = 0x00010102;
	
	if(argc != 2){
		perror("Use PNG filename in command line");
		return;
	}
	char *filename = argv[1];	
	
	printf("Opening file: %s\n", filename);

	FILE *f = readpng_or_exit(filename,&pngdata);	

	printf("This image can store %d bytes (%.2f MBs).\n",getDriveSize(&pngdata),getDriveSize(&pngdata)/(1024*1024.0f));
	
	fclose(f);


	loadDriveData(&pngdata);

	printf(pngdata.drivedata);
	sprintf(pngdata.drivedata,"Test!! I hope this will work! \n");

	saveDriveData(&pngdata);


	
	writepng("out.png",&pngdata);

}*/
