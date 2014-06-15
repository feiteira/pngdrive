#define DEFAULT_MASK 0x0010103

// returns number a bits set on an integer
int numberOfSetBits(int i);

// returns a 32bits integer from an hexadecimal string starting with 0x
int intFromHexString(char *hexstr);


// returns a pointer to a list of integers each with only one bit set, for each of the bits set in intmask
// the last element is 0 (zero)
int *bitmasks(int intmask);

