
#include "rdfs.h"
#include "rdfs_config.h"
#include "network.h"
#include <map>

using namespace std;
int master_kernel_sock_fd;
int master_userspace_sock_fd;
struct slave_context * slave_ctx[MAX_SLAVE_NUMS];
map<int,struct rdfs_file*> file_map; 
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
int rdfsOpenFile(const char* _path, int flags)
{
	struct rdfs_message message;
	message.m_type = RDFS_CLIENT_OPEN;
	sprintf(&message.m_data,"%d:%s",flags,_path);
	printf("message:%s\n",message.m_data);
	send_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	recv_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	int open_fd,inode_ino;
	long file_size,offset;
	sscanf(&message.m_data,"%d:%d:%ld:%ld",&open_fd,&inode_ino,&file_size,&offset);
	struct rdfs_file* file_p = (struct rdfs_file*)malloc(sizeof(struct rdfs_file));
	file_p->inode_ino = inode_ino;
	file_p->file_size = file_size;
	file_p->offset = offset;
	file_map.insert(make_pair(open_fd,file_p));
	return open_fd;
}
int rdfsCloseFile(int rdfs_fd)
{
	struct rdfs_message message;
	message.m_type = RDFS_CLIENT_CLOSE;
	sprintf(&message.m_data,"%d",rdfs_fd);
	send_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	recv_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	map<int,struct rdfs_file*>::iterator it;
	it = file_map.find(rdfs_fd);
	free(it->second);
	file_map.erase(it);
	return 0;
}
int rdfs_io(int rdfs_fd, const void* buffer, uint64_t length, uint64_t offset,int io_flag)
{
	struct rdfs_message message;
	message.m_type = RDFS_CLIENT_WRITE;
	map<int,struct rdfs_file*>::iterator it;
	it = file_map.find(rdfs_fd);
	int inode_ino = it->second->inode_ino;

	sprintf(&message.m_data,"%d:%ld:%ld",inode_ino,offset,length);
	send_data(sock_fd,&message,sizeof(struct rdfs_message),0);
	struct rdfs_search_message s_message;
	recv_data(sock_fd,&s_message,sizeof(struct rdfs_search_message),0);
	int nums,s_id,block_id,block_offset,size;
	nums = s_message.nums;
	int i;
	unsigned long local_addr = rdfs_mapping_address();
	for(i=0;i<nums;i++)
	{
		s_id = s_message.info[0][i];
		block_id = s_message.info[1][i];
		block_offset = s_message.info[2][i];
		size = s_message.info[3][i];
		ret = rdfs_rdma_block_rw(local_addr,s_id,block_id,block_offset,size,io_flag);
		local_addr += ret;
	}
	return length;
}
int rdfsWrite(int rdfs_fd, const void* buffer, uint64_t length, uint64_t offset)
{
	return rdfs_io(rdfs_fd,buffer,length,offset,RDFS_WRITE);
}


int rdfsRead(int rdfs_fd, void* buffer, uint64_t size, uint64_t offset)
{
	return rdfs_io(rdfs_fd,buffer,length,offset,RDFS_READ);
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

int rdfsDisconnect(rdfs fs)
{
	return 0;
}
int rdfsListDirectory(rdfs fs, const char* _path, rdfsfilelist *list)
{

}
/*
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


*/