#ifndef _Included_unity_api_header
#define _Included_unity_api_header
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char* data;
	int len;
	int width;
	int height;
} Page;

typedef struct
{
	Page* pages;
	int count;
} PageList;

int DrawPdfPages(char* filename, PageList* pageList);

#ifdef __cplusplus
}
#endif
#endif