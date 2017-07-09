/*
 * Copyright (c) 2017, The Bumblebee Project
 * Author: Stefan Helmert AKA "TheTesla" <stefan.helmert@t-online.de>
 *
 * This file is part of Bumblebee.
 *
 * Bumblebee is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bumblebee is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bumblebee. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * findpathwild.h: Bumblebee wildcard handler for configuration file handler 
 */



#define MAX_STR_LEN 512

char* getRootPath(char *rootPath, char *wildPath);
char* splitStr(char* str, char delim);
int cmpStrWild(char* inputStr, char* wildStr);
char* findPathWild(char* foundPath, char* wildPath);
char* findPathListWildDelim(char* foundPathList, char* wildPathList, char delim);
char* findPathListWild(char* foundPathList, char* wildPathList);
char* findFileWild(char* foundDriver, char* fileNameWild, char* rootPath);
char* findDriverWild(char* foundDriver, char* driverNameWild);





