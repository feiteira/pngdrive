#include <stdio.h>
#include <stdlib.h>

// counts mumber of bits sets in an integer
int numberOfSetBits(int i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// 32 bits integer from string starting with 0x
int intFromHexString(char *hexstr){
	return (int) strtol(hexstr,NULL,0);
}

int * bitmasks(int intmask){
	int * ret;
	int cnt;
	int nmaskedbits = numberOfSetBits(intmask);

	ret = malloc(sizeof(int)*(nmaskedbits+1));

	int *iter = ret;
	for(cnt = 0; cnt < 32; cnt++){// iterates over all bits
		int m = (0x1 << cnt);
//		printf(">> %x :: %x\n",m,intmask);
		if((m & intmask) != 0){// if bit is on
			*iter = m;
			iter++;
		}
	}

	*iter = 0; // last item is always 0
	return ret;
}

/*
int main(int argc, char* argv[]){
//	printf("argc %d\n", argc);
	if(argc == 1) {
		printf("Usage: ./bitmasks <hexadecimal number>\n");
		return 0;
	}

	char * str = argv[1];
	printf("Received hex value: %s\n", str );

	printf("Int value is: %d\n", intFromHexString(str));
	printf("Which has %d bits set.\n", numberOfSetBits( intFromHexString(str)));


	printf("Single Bit Masks\n");
	int *masks = bitmasks( intFromHexString(str) );
	for(int * mask = masks; *mask != 0; mask++){
		printf("\t0x%x (%d)\n", *mask,*mask);
	}
	
	
}

*/
