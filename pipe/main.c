#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <assert.h>


#define max(x, y) ( (x) > (y) ? (x) : (y) )


struct tube
{
	size_t size;
	size_t bufsize;
	int r;
	int w;
	char *pointer;
	char *buf;
};


void server_par (struct tube *, fd_set, fd_set, int, int);
void child_server (struct tube);
int degree (int);


int main(int argc, char *argv[])
{
	int n;
	int fd1[2], fd2[2], in;
	
	if (argc != 3)
	{
		printf("Use %s (number of children) (file name)\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	n = atoi(argv[1]);
	if (n < 0)
	{
		printf("Wrong number of children\n");
		exit(EXIT_FAILURE);
	}
	
	struct tube *parrent;
	struct tube child;
	parrent = malloc(n * sizeof(*parrent));
	
	int mfd = 0;
	
	pid_t pid, mainpid;
	mainpid = getpid ();
	
	fd_set readers, writers;
	FD_ZERO(&readers);
	FD_ZERO(&writers);
	
	int i;
	for (i=0; i<n; i++)
	{
		if (!i)
		{
			in = open(argv[2], O_RDONLY);
			if (in == -1)
			{
				printf("Can't open file\n");
				exit(EXIT_FAILURE);
			}
		}
		
		assert(pipe(fd1) != -1);
		assert(pipe(fd2) != -1);
		
		int size_of_buff = degree(n-i+4);
			
		pid = fork();
		
		assert (!(pid < 0));
		
		if (pid > 0)
		{
			close(fd1[1]);
			close(in);
			
			parrent[i].r = fd1[0];
			
			if (i<n-1) 
				parrent[i].w = fd2[1];
			else
			{
				parrent[i].w = STDOUT_FILENO;
				close(fd2[1]);
			}
			
			assert (fcntl(parrent[i].r, F_SETFL, O_NONBLOCK) != -1);
			assert (fcntl(parrent[i].w, F_SETFL, O_NONBLOCK) != -1);
			
			FD_SET(parrent[i].r, &readers);
			
			parrent[i].bufsize = size_of_buff;
			parrent[i].buf = malloc(parrent[i].bufsize);
			parrent[i].pointer = parrent[i].buf;
			parrent[i].size = 0;
			
			mfd = max(mfd, max(fd1[0]+1, fd2[1]+1));
		}
		else
		{
			close(fd1[0]);
			close(fd2[1]);
			
			
			child.bufsize = size_of_buff;
			child.buf = malloc(child.bufsize);
			child.w = fd1[1];
			child.r = in;
			
			for (int j =0 ;j<= i; j++)
			{
				close(parrent[j].r);
				close(parrent[j].w);
				free(parrent[j].buf);
			}
			free(parrent);
			
			break;
		}
		
		if (i != n-1)
			in = fd2[0];
	}
	
	
	if (getpid() == mainpid)
	{
		close(child.r);
		close(child.w);
		
		server_par (parrent, readers, writers, n, mfd);
	}
	else child_server (child);	
		
	return 0;
}


void server_par (struct tube *parrent, fd_set readers, fd_set writers, int n, int mfd)
{
	fd_set r, w;
	FD_ZERO(&w);
	FD_ZERO(&r);
	
	struct timeval timeout;
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;
	
	int i;
	int counter = n;
		
	while(counter)
	{
		r = readers;
		w = writers;
			
		int s = select(mfd, &r, &w, NULL, &timeout);
		
		if (s == 0)
		{
			printf("Select timeout\n");
			exit(EXIT_FAILURE);
		}
		assert (!(s < 0));
			
		for (i=0; i<n; i++)
		{
			if(FD_ISSET(parrent[i].r, &r))
			{
				parrent[i].size = read(parrent[i].r, parrent[i].buf, parrent[i].bufsize);
				assert(!(parrent[i].size < 0));
						
				FD_CLR(parrent[i].r, &readers);
					
				if (parrent[i].size == 0)
				{
					close(parrent[i].r);
					close(parrent[i].w);
					
					counter --;
				}
				else FD_SET(parrent[i].w, &writers);	
			}
				
			if(FD_ISSET(parrent[i].w, &w))
			{
				int bytes = write(parrent[i].w, parrent[i].pointer, parrent[i].size);
				assert (!(bytes < 0));
						
				parrent[i].pointer += bytes;
				parrent[i].size -= bytes;
					
				if (parrent[i].size == 0)
				{
					FD_CLR(parrent[i].w, &writers);
					FD_SET(parrent[i].r, &readers);
					
					parrent[i].pointer = parrent[i].buf;
				}
			}
		}
	}
}


void child_server (struct tube child)
{
	while(1)
	{
		child.size = read(child.r, child.buf, child.bufsize);
		
		assert (!(child.size < 0));
		if (child.size == 0) 
			exit(EXIT_SUCCESS);
		
		//exit (0);	
		
		if (write(child.w, child.buf, child.size) < 1)
			exit(EXIT_FAILURE);
	}
}


int degree (int arg)
{
	int f = 1;
	for (int j=1; j<arg; j++)
	{
		f = f * 3;
		if (f > 131072) 
			return 131072;
	}
	return f;
}
