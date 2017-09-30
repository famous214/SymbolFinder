#pragma once
#ifndef UTILS_XXXX

#define UTILS_XXXX

#if defined(_LITTLE_ENDIAN)
#define NTOHS(x) (unsigned short)((unsigned short)x << 8 | (unsigned short)x >> 8)
#elif defined (_BIG_ENDIAN)
#define NTOHS(x)	(x)
#else
#error	"Please fix _LITTLE_ENDIAN define"
#endif

#if defined(_LITTLE_ENDIAN)
#define NTOHL(x) (unsigned int)((unsigned int)x << 24 | ((unsigned int)(unsigned int)x & 0x0000ff00) << 8 | ((unsigned int)(unsigned int)x & 0x00ff0000) >> 8 | (unsigned int)x >> 24)
#elif defined (_BIG_ENDIAN)
#define NTOHL(x)	(x)
#else
#error	"Please fix _LITTLE_ENDIAN define"
#endif

#endif