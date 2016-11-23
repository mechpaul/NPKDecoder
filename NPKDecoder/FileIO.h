#include <zlib.h>

/* FILE READING */
long GetEOF(FILE* PatchFile);
long GetEOFFile(char* fileName);
long FileExists(char* fileName);
int rPackedInt(FILE* fileObj);
int rS32(FILE* fileObj);
unsigned char* ReadTheFile(char* fileName);
void WriteTheFile(char* fileName, unsigned char* toWrite, int lenFile);
char* SwitchSuffix(char* fileName, char* suffix);
unsigned char* GetFile(FILE* patchFile, int beginOffset, int lenFile);
/* END FILE READING */

/* ZLIB */
unsigned char* UnpackZlibBlock(unsigned char* zlibBlock, int lenZlib, int* sizeUnpackedZlib);
unsigned char* PackZlibblock(unsigned char* pixelData, int lenZlib, int* sizePackedZlib);
void VerifyZlibState(int zError, z_stream zlibStream);
/* END ZLIB */

/* PLATFORM DEPENDENT */
void CreateNewDirectory(char* fileName); //Possible Windows/UNIX clashing here
void CreateNewDirectoryIterate(char* dirName);
void WriteCarriageReturn(FILE* fileName); //Possible Windows/UNIX clashing here
/* END PLATFORM DEPENDENT */

/* SAFE WRAPPERS */
unsigned char* safe_malloc(int size);
unsigned char* safe_realloc(unsigned char* buffer, int size);
FILE* safe_fopen(char* fileName, char* openType);
/* END SAFE WRAPPERS */