/***********************************************************************

  Copyright (c) 2017 Stefan Helmert <stefan.helmert@t-online.de>

 ***********************************************************************/


#define MAX_STR_LEN 512

char* getRootPath(char *rootPath, char *wildPath);
char* splitStr(char* str, char delim);
int cmpStrWild(char* inputStr, char* wildStr);
char* findPathWild(char* foundPath, char* wildPath);
char* findPathListWildDelim(char* foundPathList, char* wildPathList, char delim);
char* findPathListWild(char* foundPathList, char* wildPathList);
char* findFileWild(char* foundDriver, char* fileNameWild, char* rootPath);
char* findDriverWild(char* foundDriver, char* driverNameWild);





