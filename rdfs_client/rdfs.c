
#include "rdfs.h"
#include "rdfs_config.h"
#include "network.h"
int master_kernel_sock_fd;
int master_userspace_sock_fd;
struct slave_context * slave_ctx[MAX_SLAVE_NUMS];

int rdfsConnect(void)
{
	/*
	master_kernel_sock_fd = rdfs_connect(inet_addr(MASTER_IP),CLIENT_REQUEST_PORT);
	struct slave_context *client_ctx = NULL;
	if(master_kernel_sock_fd == -1)
	{
		printf("connect to master failed\n");
		return -1;
	}
	int m_type;
	rdfs_send_message(master_kernel_sock_fd,client_ctx,CLIENT_SERACH_SLAVE_INFO);
	
	rdfs_recv_message(master_kernel_sock_fd,client_ctx,&m_type);
	*/
	master_userspace_sock_fd = rdfs_connect(inet_addr(MASTER_IP),CLIENT_USERSPACE_REQUEST_PORT);
	if(master_userspace_sock_fd <= 0)
	{
		printf("failed:fd=%d\n",master_userspace_sock_fd);
		return -1;
	}
	printf("fd=%d\n",master_userspace_sock_fd);
	userspace_test(master_userspace_sock_fd);
	return 0 ;
}
int rdfsOpenFile(int sock_fd, const char* _path, int flags)
{
	struct rdfs_message message;
	message.m_type = RDFS_CLIENT_OPEN;
	sprintf(&message.m_data,"%d:%s",flags,_path);
	printf("message:%s\n",message.m_data);
	send_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	recv_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	int open_fd;
	sscanf(&message.m_data,"%d",&open_fd);
	return open_fd;
}

int rdfsCloseFile(int sock_fd, rdfsFile file)
{

}

int userspace_test(int fd)
{
	struct rdfs_message message;
	memcpy(message.m_data,"hello world!",20);
	message.m_type = 1;
	int ret = send(fd,&message,sizeof(struct rdfs_message),0);
	//char recv_buf[MAX_MESSAGE_LENGTH];
	memset(message.m_data,0,sizeof(message.m_data));
	ret = recv(fd,&message,sizeof(struct rdfs_message),0);
	printf("recv:%s type:%d\n",message.m_data,message.m_type);
	return 0;
}
int main()
{
	int fd = rdfsConnect();
	//printf("fd = %d\n",fd);
	//while(1);
    return 0;
}
/*
int rdfsDisconnect(rdfs fs)
{
	return 0;
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



int rdfsGetAttribute(rdfs fs, rdfsFile _file, FileMeta *attr)
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