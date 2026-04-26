#ifndef __COMMON_H
#define __COMMON_H


#ifdef DEBUG
	#define DEBUG_MODE 1
#else
	#define DEBUG_MODE 0
#endif

#define PR_DEBUG(fmt, ...) \
	do { \
		if (DEBUG_MODE) { \
			fprintf(stderr, "DEBUG [%s:%d]: " fmt, \
					__func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)

#define SRC_FILE "tg.sample"
// This is only for the demo version of the program,
// in reality i would get a buffer and analyze it byte by byte
#define MES_LEN 11

#ifndef __BIGGEST_ALIGNMENT__
	#define __BIGGEST_ALIGNMENT__ (sizeof(void *) * 2)
#endif


#endif // __COMMON_H
