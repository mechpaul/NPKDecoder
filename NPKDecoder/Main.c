#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NPKParser.h"
#include "FileIO.h"
#include "iniparser.h"

#define VERSION "0.1"
#define INIFILE "NPKConfig.ini"

void PrintMenu(void);
void ParseFile(char* fileName);


int main(void)
{
	NPKConfig = iniparser_load(INIFILE);

	if(FileExists(FILE_LIST) == 1)
	{
		ParseFile(FILE_LIST);
	}
	else
	{
		PrintMenu();
	}

	return 0;
}

void ParseFile(char* fileName)
{
	char* fileList = NULL;
	char* stringToken;

	fileList = ReadTheFile(fileName);
	stringToken = strtok(fileList, "\r\n");

	while(stringToken != NULL)
	{
		if(stringToken[0] != '-' && stringToken[1] != '-')
		{
			ParseNPKFile(stringToken);
		}
		stringToken = strtok(NULL, "\r\n");
	}
}

void PrintMenu(void)
{
	printf("NPKDecoder - Version %s\nhttp://www.southperry.net/\n\n", VERSION);
	printf("HOW TO USE:\n");
	printf("Place a file called \"%s\" in the current file directory\n", FILE_LIST);
	printf("List each file that you want unpacked on a separate line.\n");
	printf("Then run this program again.\n");
}