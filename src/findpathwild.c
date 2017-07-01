/***********************************************************************

  Copyright (c) 2017 Stefan Helmert <stefan.helmert@t-online.de>

 ***********************************************************************/

#include "findpathwild.h"
#include <dirent.h>
#include <string.h>


char* getRootPath(char *rootPath, char *wildPath)
{
    char c;
    c = 1;
    unsigned pidxend = 0;
    for(int i=0; c && i < MAX_STR_LEN; i++){
      c = wildPath[i];
      rootPath[i] = c;
      if('/' == c){
        pidxend = i;
      }
      if('?' == c){
        rootPath[pidxend] = 0;
        return &wildPath[pidxend+1]; // may be dangerous, bacause reduces maximum string length
      }
    }
    return "";
}

char* splitStr(char* str, char delim)
{
    char c;
    c = 1;
    for(int i=0; c && i < MAX_STR_LEN; i++){
        c = str[i];
        if(delim == c){
            str[i] = 0;
            return &str[i+1];
        }

    }
    return "";
}

int cmpStrWild(char* inputStr, char* wildStr)
{
    char ci, cw;
    ci = 1;
    cw = 1;
    for(int i = 0; ci && cw && i < MAX_STR_LEN; i++){
        ci = inputStr[i];
        cw = wildStr[i];
        if(ci == cw) continue;
        if('?' == cw) continue; 
        return 0;
    }
    if(0 == ci && 0 == cw) return 1;
    return 0;
} 

char* findPathWild(char* foundPath, char* wildPath)
{
    DIR *dir;
    struct dirent *ent;
    char found[MAX_STR_LEN];
    char rootPath[MAX_STR_LEN];
    char currentLabel[MAX_STR_LEN];
    char* pathEnd;
    int cmpResult;


    if('/' == wildPath[strlen(wildPath) -1]){
        wildPath[strlen(wildPath) -1] = 0;
    }

    pathEnd = getRootPath(rootPath, wildPath);
    strncpy(currentLabel, pathEnd, MAX_STR_LEN - 1);    
    pathEnd = splitStr(currentLabel, '/');    
    strncat(rootPath, "/", MAX_STR_LEN - strlen(rootPath) - 1);
    // structur:   /root/Path / currentLabel / p?th/E?d/
    //           | rootPath   | currentLabel | pathEnd

    if ((dir = opendir (rootPath)) != NULL) { // acceppt only if path exists 
        if(0 == currentLabel[0]) { // no currentLabel means end of path reached
            strncpy(foundPath, rootPath, MAX_STR_LEN - 1);
            closedir (dir);
            return foundPath; // means the path is found in deepest recursion
        }
        while ((ent = readdir (dir)) != NULL) {  // go through all dirs in rootPath
            cmpResult = cmpStrWild(ent->d_name, currentLabel); // check if wildcard matches
            if(cmpResult) {  // found match - next recursion step, process inner directory wildcards
                strncpy(found, rootPath, MAX_STR_LEN - 1);
                strncat(found, ent->d_name, MAX_STR_LEN - strlen(found) - 1);
                strncat(found, "/", MAX_STR_LEN - strlen(found) - 1);
                strncat(found, pathEnd, MAX_STR_LEN - strlen(found) - 1);
                if(findPathWild(foundPath, found) != NULL) {
                    closedir (dir);
                    return foundPath;
                }
            }
        }
        closedir (dir);
        return NULL;
    } else {
        return NULL;
    }
}


char* findPathListWildDelim(char* foundPathList, char* wildPathList, char delim)
{
    char lWildPathList[MAX_STR_LEN] = "";
    char foundPath[MAX_STR_LEN] = "";
    char* nextStr;
    char* wildPath;
    char delimStr[2];
    delimStr[0] = delim;
    delimStr[1] = 0;
    foundPathList[0] = 0;
    strncpy(lWildPathList, wildPathList, MAX_STR_LEN - 1);
    wildPath = lWildPathList;
    while(1){
        nextStr = splitStr(wildPath, delim);

        if(0 == wildPath[0]) {
            return foundPathList;
        }

        if(findPathWild(foundPath, wildPath)){
            if(0 != foundPathList[0]) {
                strncat(foundPathList, delimStr, MAX_STR_LEN - strlen(foundPathList) - 1);
            }
            strncat(foundPathList, foundPath, MAX_STR_LEN - strlen(foundPathList) - 1);
        } 
        wildPath = nextStr;
    }
    return NULL;
}

char* findPathListWild(char* foundPathList, char* wildPathList)
{
    return findPathListWildDelim(foundPathList, wildPathList, ':');
}

