/****************************************************************
*
* 
*
* g++ -o rdfs rdfs.cpp test.cpp -std=c++14 -lpthread
*
****************************************************************/
#ifndef _RDFS_LIB_H
#define _RDFS_LIB_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "network.h"

int rdfsConnect(void);
/*
int rdfsDisconnect(rdfs fs);

rdfsFile rdfsOpenFile(rdfs fs, const char* path, int flags);

int rdfsCloseFile(rdfs fs, rdfsFile file);

int rdfsMknod(rdfs fs, const char* path);

int rdfsAccess(rdfs fs, const char* path);

int rdfsGetAttribute(rdfs fs, rdfsFile file, FileMeta *attr);

int rdfsWrite(rdfs fs, rdfsFile file, const void* buffer, uint64_t size, uint64_t offset);

int rdfsRead(rdfs fs, rdfsFile file, void* buffer, uint64_t size, uint64_t offset);

int rdfsCreateDirectory(rdfs fs, const char* path);

int rdfsDelete(rdfs fs, const char* path);
int rdfsFreeBlock(uint16_t node_id, uint64_t startBlock, uint64_t  countBlock);

int rdfsRename(rdfs fs, const char* oldpath, const char* newpath);

int rdfsListDirectory(rdfs fs, const char* path, rdfsfilelist *list);

int rdfsTest(rdfs fs, int offset);
int rdfsRawWrite(rdfs fs, rdfsFile file, const void* buffer, uint64_t size, uint64_t offset);
int rdfsRawRead(rdfs fs, rdfsFile file, void* buffer, uint64_t size, uint64_t offset);
int rdfsRawRPC(rdfs fs);
*/
#endif