#include "iniparser.h"

#define FILE_LIST "NPKList.txt"

dictionary* NPKConfig;

struct NPKImage
{
	int mode;               // How the image is byte compressed - 3 modes (0x0E = 1555, 0x0F = 4444, 0x10 = 8888) - All in ARGB
	int compressed;         // If the image is zlib compressed
	int width;              // Width of the image
	int height;             // height of the image
	int lengthCompressed;   // Compressed length
	int lengthDecompressed; // Decompressed length
	int key_x;              // Represents the X coordinate of the center of the image - used for animations
	int key_y;              // Represents the Y coordinate of the center of the image - used for animations
	int max_width;          // Unknown
	int max_height;			// Unknown
	unsigned char* pixelData; // The compressed or file-read pixel data
	unsigned char* expandedPixelData; // The expanded pixel data, according to the mode
};

void ParseNPKFile(char* stringToken);
void ReadHeaderString(char* fileName, FILE* npkFile);
void ParseDirTree(char* fileName, FILE* npkFile, int baseOffset);
void ParseImgFile(char* fileName, FILE* npkFile, int baseOffset, int numEntries);
unsigned char* Expand(unsigned char* pixelData, int width, int height, int lengthDecompressed, int mode);
void ToPng(char* outputFileName, unsigned char* image, int height, int width);
unsigned int GetInt(FILE* npkFile);
unsigned short GetShort(FILE* npkFile);
int endswith(char* fileName, char* checkEnd);