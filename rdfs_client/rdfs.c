
#include "rdfs.h"
#include "rdfs_config.h"
#include "network.h"
int master_sock_fd;
struct slave_context * slave_ctx[MAX_SLAVE_NUMS];

int rdfsConnect(const char* host, int port)
{
	master_sock_fd = rdfs_connect(inet_addr(host),port);
	struct slave_context *client_ctx = NULL;
	if(master_sock_fd == -1)
	{
		printf("connect to master failed\n");
		return -1;
	}
	int m_type;
	rdfs_send_message(master_sock_fd,client_ctx,CLIENT_SERACH_SLAVE_INFO);
	//int m_type;
	rdfs_recv_message(master_sock_fd,client_ctx,&m_type);

	return master_sock_fd ;
}
int main()
{
	int fd = rdfsConnect("192.168.0.2",18514);
    printf("fd = %d\n",fd);
    return 0;
}
/*
int rdfsDisconnect(rdfs fs)
{
	return 0;
}

void correct(const char *old_path, char *new_path)
{

}

bool getParentDirectory(const char *path, char *parent)
{ 
   
}


bool getNameFromPath(const char *path, char *name) 
{ 

}

int rdfsAddMetaToDirectory(rdfs fs, char* parent, char* name, bool isDirectory)
{
 
}

int rdfsRemoveMetaFromDirectory(rdfs fs, char* path, char* name)
{

}

int rdfsMknodWithMeta(rdfs fs, char *path, FileMeta *metaFile)
{

}

rdfsFile rdfsOpenFile(rdfs fs, const char* _path, int flags)
{

}

int rdfsMknod(rdfs fs, const char* _path)
{

}

int rdfsCloseFile(rdfs fs, rdfsFile file)
{

}

int rdfsGetAttribute(rdfs fs, rdfsFile _file, FileMeta *attr)
{

}

int rdfsAccess(rdfs fs, const char* _path)
{

}

int rdfsWrite(rdfs fs, rdfsFile _file, const void* buffer, uint64_t size, uint64_t offset)
{

}


int rdfsRead(rdfs fs, rdfsFile _file, void* buffer, uint64_t size, uint64_t offset)
{

}


#define RECURSIVE_CREATE 1
#ifdef RECURSIVE_CREATE
int rdfsCreateDirectory(rdfs fs, const char* _path)
{

}
#else
int rdfsCreateDirectory(rdfs fs, const char* _path)
{
	
}
#endif

int rdfsDelete(rdfs fs, const char* _path)
{
	
}
int rdfsFreeBlock(uint16_t nodeHash, uint64_t startBlock, uint64_t countBlock)
{

}
int renameDirectory(rdfs fs, const char *oldPath, const char *newPath)
{

}

int rdfsRename(rdfs fs, const char* _oldpath, const char* _newpath)
{

}

int rdfsListDirectory(rdfs fs, const char* _path, rdfsfilelist *list)
{

}

int rdfsTest(rdfs fs, const int offset)
{

}

int rdfsRawWrite(rdfs fs, rdfsFile _file, const void* buffer, uint64_t size, uint64_t offset)
{

	return 0;
}
int rdfsRawRead(rdfs fs, rdfsFile _file, void* buffer, uint64_t size, uint64_t offset)
{
	
	return 0;
}
*/