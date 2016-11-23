#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <windows.h>

#include "FileIO.h"

#define BLOCKSIZE 0xF0000

/* FILE READING */

long GetEOF(FILE* PatchFile)
{
	long remember;
	long eof;

	remember = ftell(PatchFile);
	fseek(PatchFile, 0, SEEK_END);
	eof = ftell(PatchFile);
	fseek(PatchFile, remember, SEEK_SET);

	return eof;
}

long FileExists(char* fileName)
{
	FILE* fileptr;

	fileptr = fopen(fileName, "rb");
	if(fileptr == NULL)
	{
		return 0;
	}
	else
	{
		fclose(fileptr);
		return 1;
	}
}

long GetEOFFile(char* fileName)
{
	FILE* genericFile;
	int eof;

	genericFile = safe_fopen(fileName, "rb");
	eof = GetEOF(genericFile);
	fclose(genericFile);

	return eof;
}

int rS32(FILE* fileObj)
{
	int readValue;

	fread(&readValue, 4, 1, fileObj);

	return readValue;
}

int rPackedInt(FILE* fileObj)
{
	int readValue = 0;

	fread(&readValue, 1, 1, fileObj);
	if((char)readValue == -128)
	{
		fread(&readValue, 4, 1, fileObj);
	}
	return readValue;
}

unsigned char* ReadTheFile(char* fileName)
{
	FILE* genericFile;
	int eof;
	unsigned char* wholeFile;

	genericFile = safe_fopen(fileName, "rb");
	eof = GetEOF(genericFile);
	wholeFile = malloc(eof + 1);
	fread(wholeFile, eof, 1, genericFile);
	wholeFile[eof] = '\0';
	fclose(genericFile);

	return wholeFile;
}

void WriteTheFile(char* fileName, unsigned char* toWrite, int lenFile)
{
	FILE* writeFile;

	writeFile = safe_fopen(fileName, "wb");
	fwrite(toWrite, lenFile, 1, writeFile);
	fclose(writeFile);
}

char* SwitchSuffix(char* fileName, char* suffix)
{
	int i;
	int lenName;
	char* returnLocation = NULL;

	lenName = strlen(fileName);

	for(i = 0; i < lenName; i++)
	{
		if(fileName[i] == '.')
		{
			returnLocation = malloc(i + strlen(suffix) + 1);
			memcpy(returnLocation, fileName, i);
			memcpy(returnLocation + i, suffix, strlen(suffix) + 1);
			returnLocation[strlen(suffix) + i] = '\0';
			break;
		}
		if(i+1 == lenName)
		{
			//Okay, so it doesn't have a suffix, let's give it one!
			i++;
			returnLocation = malloc(i + strlen(suffix) + 1);
			memcpy(returnLocation, fileName, i);
			memcpy(returnLocation + i, suffix, strlen(suffix) + 1);
			returnLocation[strlen(suffix) + i] = '\0';
			break;
		}
	}
	
	if(returnLocation == NULL)
	{
		printf("Switchsuffix failed with string %s and suffix %s\n", fileName, suffix);
		exit(EXIT_FAILURE);
	}

	return returnLocation;
}

unsigned char* GetFile(FILE* patchFile, int beginOffset, int lenFile)
{
	unsigned char* internalFile;

	fseek(patchFile, beginOffset, SEEK_SET);
	internalFile = safe_malloc(lenFile);
	fread(internalFile, lenFile, 1, patchFile);

	return internalFile;
}

/* END FILE READING */

/* ZLIB */

unsigned char* UnpackZlibBlock(unsigned char* zlibBlock, int lenZlib, int* unpackedLenZlib)
{
	unsigned char* pDest = safe_malloc(lenZlib);
	int lengthpDest = lenZlib;

	unsigned char* out = safe_malloc(lenZlib / 2);
	int blockSize = lenZlib / 2;

	int lengthOfBlock;
	int numRounds = 0;	
	int zError;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = Z_NULL;
	strm.avail_in = 0;
	zError = inflateInit(&strm);

	*unpackedLenZlib = 0;

	do{
		//Set the zlib stream to the block to be decompressed
		strm.avail_in = lenZlib;
		strm.next_in = zlibBlock;
		do{
			//avail_out is the size of the output buffer
			//next_out is the location of the output buffer
			strm.avail_out = blockSize;
			strm.next_out = out;

			zError = inflate(&strm, Z_NO_FLUSH);
			VerifyZlibState(zError, strm);

			//strm.avail_out is the amount of data unconsumed by the zlib unpacking (the length
			//of invalid data in "out")
			lengthOfBlock = blockSize - strm.avail_out;
			*unpackedLenZlib += lengthOfBlock;
			
			//Ensure that pDest has enough space remaining for the newly unpacked stream
			if(*unpackedLenZlib > lengthpDest)
			{
				pDest = safe_realloc(pDest, lengthpDest * 2);
				lengthpDest *= 2;
			}

			//Copy from the unpacked zlib stream to pDest
			memcpy(pDest + (blockSize * numRounds), out, lengthOfBlock);
			numRounds++;
		} while(strm.avail_out == 0);
	} while(zError != Z_STREAM_END);

	inflateEnd(&strm);

	free(out);

	//Ensure that pDest isn't taking up more memory than it should (clip off
	//the unused data at the end)
	if(*unpackedLenZlib < lengthpDest)
	{
		pDest = safe_realloc(pDest, *unpackedLenZlib);
	}

	return pDest;
}

unsigned char* PackZlibblock(unsigned char* pixelData, int lenZlib, int* packedLenZlib)
{
	unsigned char* pDest = safe_malloc(lenZlib);
	int lengthpDest = lenZlib;

	unsigned char* out = safe_malloc(lenZlib / 2);
	int blockSize = lenZlib / 2;

	int lengthOfBlock;
	int numRounds = 0;	
	int zError;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = Z_NULL;
	strm.avail_in = 0;
	zError = deflateInit(&strm, 9);

	*packedLenZlib = 0;

	do{
		//Set the zlib stream to the block to be decompressed
		strm.avail_in = lenZlib;
		strm.next_in = pixelData;
		do{
			//avail_out is the size of the output buffer
			//next_out is the location of the output buffer
			strm.avail_out = blockSize;
			strm.next_out = out;

			zError = deflate(&strm, Z_NO_FLUSH);
			VerifyZlibState(zError, strm);

			//strm.avail_out is the amount of data unconsumed by the zlib unpacking (the length
			//of invalid data in "out")
			lengthOfBlock = blockSize - strm.avail_out;
			*packedLenZlib += lengthOfBlock;
			
			//Ensure that pDest has enough space remaining for the newly unpacked stream
			if(*packedLenZlib > lengthpDest)
			{
				pDest = safe_realloc(pDest, lengthpDest * 2);
				lengthpDest *= 2;
			}

			//Copy from the unpacked zlib stream to pDest
			memcpy(pDest + (blockSize * numRounds), out, lengthOfBlock);
			numRounds++;
		} while(strm.avail_out == 0);
	} while(zError != Z_STREAM_END);

	inflateEnd(&strm);

	free(out);

	//Ensure that pDest isn't taking up more memory than it should (clip off
	//the unused data at the end)
	if(*packedLenZlib < lengthpDest)
	{
		pDest = safe_realloc(pDest, *packedLenZlib);
	}

	return pDest;
}

void VerifyZlibState(int zError, z_stream zlibStream)
{
	switch(zError)
	{
	case Z_STREAM_ERROR: /* And fall through */
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
		printf("Zlib Error %d\nSolution\nRedownload the patch file (might be corrupt)\n", zError);
		MessageBox(NULL, "Zlib clobbered.", "Error", MB_ICONEXCLAMATION | MB_OK);
		inflateEnd(&zlibStream);
		exit(EXIT_FAILURE);
	}
}

/* END ZLIB */

/* PLATFORM DEPENDENT */

//Possible Windows/UNIX clashing here
void CreateNewDirectory(char* fileName)
{
	CreateDirectory(fileName, NULL);
}

void CreateNewDirectoryIterate(char* dirName)
{
	char realDirName[300];
	unsigned int i;

	for(i = 0; dirName[i] != '\0'; i++)
	{
		if((dirName[i] == '/') || (dirName[i] == '\\'))
		{
			memcpy(realDirName, dirName, i+1);
			realDirName[i+1] = '\0';
			CreateNewDirectory(realDirName);
		}
	}
}

//Possible Windows/UNIX clashing here
void WriteCarriageReturn(FILE* fileName)
{
	fwrite("\r\n", 2, 1, fileName);
}

/* END PLATFORM DEPENDENT */

/* SAFE WRAPPERS */

unsigned char* safe_malloc(int size)
{
	unsigned char* block;

	block = malloc(size);
	if(block == NULL)
	{
		printf("Out of memory!\nTried to allocate %d bytes.\n", size);
		MessageBox(NULL, "Out of memory!", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}
	return block;
}

unsigned char* safe_realloc(unsigned char* buffer, int size)
{

	buffer = realloc(buffer, size);
	if(buffer == NULL)
	{
		printf("Out of memory reallocation!\nTried to reallocate %d bytes.\n", size);
		MessageBox(NULL, "Out of realloc memory!", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}
	return buffer;
}

FILE* safe_fopen(char* fileName, char* openType)
{
	FILE* genericFile;

	genericFile = fopen(fileName, openType);
	if(genericFile == NULL)
	{
		printf("File %s does not exist\n", fileName);
		MessageBox(NULL, "Could not find a file.\nSolution\nAre you sure NXPatcher is running from your Maplestory directory?\n", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}
	return genericFile;
}

/* SAFE WRAPPERS END */