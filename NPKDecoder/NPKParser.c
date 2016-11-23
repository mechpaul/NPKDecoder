#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FileIO.h"
#include "NPKParser.h"
#include <png.h>
#include "iniparser.h"

#define OUTPUT_DIR "ImageUnpack"

void ParseNPKFile(char* fileName)
{
	FILE* npkFile;

	npkFile = safe_fopen(fileName, "rb");

	//Clip off the ".npk"
	fileName[strlen(fileName) - 4] = '\0';

	ReadHeaderString(fileName, npkFile);

	fclose(npkFile);
}

void ReadHeaderString(char* fileName, FILE* npkFile)
{
	char stringBuf[50] = {0};
	int nulls;
	int numEntries;
	char eachChar = 1;
	int stringBufPos = 0;

	while(eachChar != 0)
	{
		eachChar = fgetc(npkFile);
		stringBuf[stringBufPos++] = eachChar;
	}

	if(strcmp(stringBuf, "NeoplePack_Bill") == 0)
	{
		ParseDirTree(fileName, npkFile, ftell(npkFile));
	}
	else if(strcmp(stringBuf, "Neople Image File") == 0)
	{
		nulls = GetInt(npkFile);
		nulls = GetShort(npkFile);
		nulls = GetInt(npkFile); //No idea what this is - always 0x00000001
		numEntries = GetInt(npkFile);
		ParseImgFile(fileName, npkFile, ftell(npkFile), numEntries);
	}
	else
	{
		printf("Strange string at %d?", ftell(npkFile));
		exit(EXIT_FAILURE);
	}
}

void ParseImgFile(char* folderName, FILE* npkFile, int baseOffset, int numEntries)
{
	struct NPKImage thisImage;
	char fileName[512] = {0};
	char Number[5] = {0};
	unsigned char* picBuf;
	int i;
	int eof;
	int imageLink;
	int link = 0;
	int extractUncompressed = iniparser_getboolean(NPKConfig, "npkconfig:extractuncompressedimages", 0);
	int npklinkbacks = iniparser_getboolean(NPKConfig, "npkconfig:npklinkbacks", 0);

	for(i = 0; i < numEntries; i++)
	{
		thisImage.mode = GetInt(npkFile);
		if(thisImage.mode == 0x11)
		{
			//This is a back reference
			//This will ALWAYS reference a file previously in a directory
			//never goes forward in a directory
			imageLink = GetInt(npkFile);
			if(npklinkbacks)
			{
				memcpy(fileName, folderName, strlen(folderName) + 1);
				_itoa(imageLink, Number, 10);
				strcat(fileName, Number);
				strcat(fileName, ".png");

				picBuf = ReadTheFile(fileName);
				eof = GetEOFFile(fileName);

				memcpy(fileName, folderName, strlen(folderName) + 1);
				_itoa(i, Number, 10);
				strcat(fileName, Number);
				strcat(fileName, ".png");

				WriteTheFile(fileName, picBuf, eof);

				free(picBuf);
			}
			continue;
		}
		thisImage.compressed = GetInt(npkFile) - 5; //6 = Compressed, 5 = Not compressed
		thisImage.width = GetInt(npkFile);
		thisImage.height = GetInt(npkFile);
		thisImage.lengthCompressed = GetInt(npkFile);
		thisImage.key_x = GetInt(npkFile);
		thisImage.key_y = GetInt(npkFile);
		thisImage.max_width = GetInt(npkFile);
		thisImage.max_height = GetInt(npkFile);
		
		picBuf = calloc(thisImage.lengthCompressed, 1);

		switch (thisImage.compressed)
		{
		case 1:
			fread(picBuf, thisImage.lengthCompressed, 1, npkFile);
			thisImage.pixelData = UnpackZlibBlock(picBuf, thisImage.lengthCompressed, &thisImage.lengthDecompressed);
			thisImage.expandedPixelData = Expand(thisImage.pixelData, thisImage.width, thisImage.height, thisImage.lengthDecompressed, thisImage.mode);
			free(thisImage.pixelData);
			free(picBuf);
			memcpy(fileName, folderName, strlen(folderName) + 1);
			_itoa(i, Number, 10);
			strcat(fileName, Number);
			strcat(fileName, ".png");
			CreateNewDirectoryIterate(fileName);
			ToPng(fileName, thisImage.expandedPixelData, thisImage.height, thisImage.width);
			break;
		case 0:
			fread(picBuf, thisImage.lengthCompressed / 2, 1, npkFile);
			if (extractUncompressed == 1)
			{
				thisImage.expandedPixelData = Expand(picBuf, thisImage.width, thisImage.height, thisImage.lengthCompressed, thisImage.mode);
				free(picBuf);
				memcpy(fileName, folderName, strlen(folderName) + 1);
				_itoa(i, Number, 10);
				strcat(fileName, Number);
				strcat(fileName, ".png");
				CreateNewDirectoryIterate(fileName);
				ToPng(fileName, thisImage.expandedPixelData, thisImage.height, thisImage.width);
			}
			break;
		default:
			printf("Unknown compression type at %d?\n", ftell(npkFile));
			exit(EXIT_FAILURE);
			break;
		}
	}
}

unsigned char* Expand(unsigned char* pixelData, int width, int height, int length, int mode)
{
	unsigned char* image = safe_malloc(width * height * 4);
	int imageIndex = 0;
	int pixelIndex = 0;
	unsigned char a;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	int pixel0;
	int pixel1;


	switch(mode)
	{
	case 0x0E: //1555
		for(pixelIndex = 0; pixelIndex < width * height * 2; pixelIndex += 2)
		{
			pixel0 = pixelData[pixelIndex];
			pixel1 = pixelData[pixelIndex + 1];
			a = ((pixel1 & 0x80) >> 7) * 0xFF;
			r = ((pixel1 & 0x7C) << 1) | ((pixel1 & 0x7C) >> 4);
			g = ((pixel1 & 0x03) << 6) | ((pixel0 & 0xE0) >> 2);
			g = g | (g >> 5);
			b = ((pixel0 & 0x1F) << 3) | ((pixel0 & 0x1F) >> 2);
			image[imageIndex++] = r;
			image[imageIndex++] = g;
			image[imageIndex++] = b;
			image[imageIndex++] = a;
		}
		break;
	case 0x0F: //4444
		for(pixelIndex = 0; pixelIndex < width * height * 2; pixelIndex += 2)
		{
			pixel0 = pixelData[pixelIndex];
			pixel1 = pixelData[pixelIndex + 1];
			a = pixel1 & 0xF0; 
			r = pixel1 & 0x0F; 
			g = pixel0 & 0xF0; 
			b = pixel0 & 0x0F;
			image[imageIndex++] = r | (r << 4);
			image[imageIndex++] = g | (g >> 4);
			image[imageIndex++] = b | (b << 4);
			image[imageIndex++] = a | (a >> 4);
		}
		break;
	case 0x10: //8888
		for(pixelIndex = 0; pixelIndex < width * height * 4; pixelIndex += 4)
		{
			//RGBA
			//ARGB
			image[imageIndex++] = pixelData[pixelIndex + 2];
			image[imageIndex++] = pixelData[pixelIndex + 1];
			image[imageIndex++] = pixelData[pixelIndex + 0];
			image[imageIndex++] = pixelData[pixelIndex + 3];
		}
		break;
	default:
		printf("Unknown mode type %d?\n", mode);
		exit(EXIT_FAILURE);
	}

	return image;
}

void ToPng(char* outputFileName, unsigned char* image, int height, int width)
{
	//This requires libpng and zlib

	FILE* pngFile;
	png_bytep* row_pointers;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	int i;
	int compressionLevel = iniparser_getint(NPKConfig, "npkconfig:zlibcompression", 6);

	pngFile = fopen(outputFileName, "wb");
	png_init_io(png_ptr, pngFile);

	png_set_compression_level(png_ptr, compressionLevel);
	/* write header */
	png_set_IHDR(png_ptr, info_ptr, width, height,
			8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	row_pointers = (png_bytep*) malloc((sizeof(png_bytep) * height) + 1);
	for(i = 0; i < height; i++)
	{
		row_pointers[i] = &(image[i * width * 4]);
	}

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, info_ptr);

	free(row_pointers);
	free(image);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(pngFile);
}

void ParseDirTree(char* fileName, FILE* npkFile, int baseOffset)
{
	//Iterators
	int i;
	int j;

	//Each file in the directory tree
	int numFiles;
	int absLocation;
	int size;
	char subName[256];
	char key[256] = "puchikon@neople dungeon and fighter DNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNF";
	char* newFolderName;
	int remember;
	unsigned char* wavFile;
	char decryptedByte;

	numFiles = GetInt(npkFile);
	for(i = 0; i < numFiles; i++)
	{
		absLocation = GetInt(npkFile);
		size = GetInt(npkFile);
		
		//Get encrypted string
		fread(subName, 256, 1, npkFile);
		for(j = 0; j < 256; j++)
		{
			decryptedByte = subName[j] ^ key[j];
			subName[j] = decryptedByte == '_' ? '/' : decryptedByte;
		}
		if(endswith(subName, ".img"))
		{
			//Remove ".img" from the filename
			subName[strlen(subName) - 4] = '\0';

			//Create place where all pngs will be stored
			newFolderName = safe_malloc(strlen(subName) + strlen("/") + 1);
			newFolderName[0] = '\0'; //So strcat() doesn't give us problems
			strcat(newFolderName, subName);
			strcat(newFolderName, "/");
			CreateNewDirectoryIterate(newFolderName);
			printf("%s\n", newFolderName);

			//Jump to new file
			remember = ftell(npkFile);
			fseek(npkFile, absLocation, SEEK_SET);
			ReadHeaderString(newFolderName, npkFile);
			fseek(npkFile, remember, SEEK_SET);
			free(newFolderName);
		}
		else if(endswith(subName, ".wav"))
		{
			//Create place where all pngs will be stored
			newFolderName = calloc(strlen(subName) + 1, 1);
			strcat(newFolderName, subName);
			CreateNewDirectoryIterate(newFolderName);
			printf("%s\n", newFolderName);

			//Write file
			remember = ftell(npkFile);
			fseek(npkFile, absLocation, SEEK_SET);
			wavFile = safe_malloc(size);
			fread(wavFile, size, 1, npkFile);
			fseek(npkFile, remember, SEEK_SET);
			WriteTheFile(subName, wavFile, size);
			free(wavFile);
			free(newFolderName);
		}
		else
		{
			printf("Unknown ending %s?\n", subName);
			exit(EXIT_FAILURE);
		}
	}
}

unsigned int GetInt(FILE* npkFile)
{
	unsigned int eachInt;

	fread(&eachInt, 4, 1, npkFile);

	return eachInt;
}

unsigned short GetShort(FILE* npkFile)
{
	unsigned int eachShort;

	fread(&eachShort, 2, 1, npkFile);

	return eachShort;
}

int endswith(char* fileName, char* checkEnd)
{
	return strcmp(fileName + strlen(fileName) - strlen(checkEnd), checkEnd) == 0;

}