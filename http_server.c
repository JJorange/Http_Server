```c
#include<stdio.h>
#include<string.h>
#include<stdlib.h> 
#include<unistd.h>
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include"http_server.h"
#include <sys/types.h>
#include<wait.h>
#include <fcntl.h>

typedef struct sockaddr sockaddr ;
typedef struct sockaddr_in sockaddr_in;

//一次从socket中读取一行数据
//把数据放到buf缓冲区中
//如果读取失败，返回值为-1
int ReadLine(int sock,char buf[],ssize_t size){
	   //1.从socket中一次读取一个字符
	   char c = '\0';
	   ssize_t i = 0;//当前读了多少个字符
	   //结束条件：1.读的长度太长，达到缓冲区上限。2.读到了\n(需要兼容\r \r\n的情况，如果遇到\r 或者\r\n换成\n)
	   while(i < size -1 && c != '\n'){
		      ssize_t read_size = recv(sock,&c,1,0);
			  if(read_size < 0){
				     return -1;
			  }
			  if(read_size == 0){
				     //预期读到\n这样的换行符
					 //先读到EOF，这种情况认为是失败
					 return -1;
			  }
			  if(c == '\r'){
				     //当前遇到了\r，但还需确认下一个字符是否为\n
					 //MSG_PEEK从内核的缓冲区中读数据
					 //但是得到的数据不会从缓冲区中删除
					 recv(sock,&c,1,MSG_PEEK);
					 if(c == '\n'){
						    //此时整个的分隔符就是\r\n
							recv(sock,&c,1,0);
					 }else{
						    //当前分隔符确定为\r此时转化为\n
						    c = '\n';
					 }
			  }
			  //只要上面c读到的是\r,那么if结束之后，c都变成\n
			  buf[i++] = c;
	   }
	   buf[i] = '\0';
	   return i;//真正往缓冲区放置字符的个数
}

int Splist(char input[],const char* split_char,char* output[],int output_size){
	//使用线程安全的strtok_r函数
	//strtok会创建静态变量，多线程不安全
	int i=0;
	char* tmp = NULL;
	char * pch;
	pch = strtok_r(input,split_char,&tmp);
	while(pch != NULL){
		   if(i >= output_size){
			      return i;
		   }
		   output[i++] = pch;
		   pch = strtok_r(NULL,split_char,&tmp);
	}
	return i;
}

int ParseFirstLine(char first_line[],char** p_url,char** p_method){
	//把首行按照空格进行字符串切分
	char* tok[10];
	//把first_line按照空格进行字符串切分
	//切分得到的每一个部分，就放到tok数组中
	//返回值，就是tok数组中包含几个元素
	//最有一个参数10表示tok数组最多能放几个元素
	int tok_size = Splist(first_line, " ", tok, 10);
	if(tok_size != 3){
		printf("Split failed! tok_size = %d\n", tok_size);
    return -1;
  }
	*p_method  = tok[0];
	*p_url = tok[1];
	return 0;
}

int ParseQueryString(char* url,char** p_url_path,char** p_query_string){
  *p_url_path = url;
  char* p = url;
	   for(;*p != '\0';p++){
		      if(*p == '?'){
				  *p = '\0';		 
				  *p_query_string = p+1;
				  return 0;
			  }
	   }
	   //循环结束没找到 ? ，说明这个请求不存在
	   *p_query_string = NULL;
	   return 0;  
}

int ParseHeader(int sock,int* content_length){   
	   char buf[SIZE] = {0};
	   while(1){
		   //1.循环从socket中读取一行
		   ssize_t read_size = ReadLine(sock,buf,sizeof(buf));
		   //处理读失败的情况
		   if(read_size <= 0){    
			   return -1;
		   }
		   //处理读完的情况--读到空行结束循环
		   if(strcmp(buf,"\n") == 0){
			      return 0;
		   }
		   //2.判定当前行是不是content_length
		   //  如果是content_length就直接把value读取出来
		   //  如果不是就直接丢弃
		   const char* content_length_str = "Content-Length: ";
		   if(content_length != NULL 
				   && strncmp(buf,"Content-Length: ",
					   strlen(content_length_str)) == 0){
			      *content_length = atoi(buf + strlen(content_length_str));

		   }
	   }
	  return 0;
}

void Handler404(int sock){
  //构造一个完整的HTTP响应
  //状态码就是404
  //body部分应该是一个40相关的错误页面
  const char* first_line = "HTTP/1.1 404 Not Found\n";
  const char* type_line = "Content-Type: text/html;charset=utf-8\n";
  const char* blank_line = "\n";
  const char* html = "<head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"></head>""<h1>喵喵喵</h1>";
  char content_length[1024] = {0};
  sprintf(content_length,"Content-Length: %lu\n", strlen(html));
  send(sock,first_line,strlen(first_line),0);
  send(sock,type_line,strlen(type_line),0);
  send(sock,content_length,strlen(content_length),0);
  send(sock,blank_line,strlen(blank_line),0);
  //send(sock,type_line,strlen(type_line),0);
  send(sock,html,strlen(html),0);
  return;

}

void PrintRequest(Request* req){
  printf("method: %s\n",req->method);
  printf("url_path: %s\n",req->url_path);
  printf("query_string: %s\n",req->query_string);
  printf("conten_length: %d\n",req->content_length);
  return;
}

int IsDir(const char* file_path)
{
  struct stat st;
  int ret = stat(file_path, &st);
  if(ret < 0)
  {
    return 0;
  }
  if (S_ISDIR(st.st_mode))
  {
    return 1;
  }
  return 0;
}

void HandlerFilePath(const char* url_path, char file_path[])
{
  
  //1)给url_path加上前缀（HTTP服务器的根目录）
  //url_path => /index.html 
  //file_path => ./wwwroot/index.html 
  sprintf(file_path,"./wwwroot%s",url_path);
  //2)例如url_path => /,此时url_path其实是一个目录
  //如果是目录的话，就给这个目录之中追加一个index.html 
  //url_path是 / 或者 /image/的情况
  if(file_path[strlen(file_path) - 1] == '/')
  {
    strcat(file_path,"index.html");
  }
  //c)例如url_path => /image
  if(IsDir(file_path))
  {
    strcat(file_path, "/index.html");
  }
  return;
  
}

ssize_t GetFileSize(const char* file_path)
{
  struct stat st;
  int ret = stat(file_path, &st);
  if(ret < 0)
  {
    //打开文件失败，很可能是文件不存在
    //此时直接返回文件长度为零
    return 0;
  }
  return st.st_size;
}

int WriteStaticFile(int sock,const char* file_path)
{
  //1.打开文件
  //打开文件失败
  int fd = open(file_path, O_RDONLY);
  if(fd < 0)
  {
    perror("open");
    return 404;
  }
  //2.把构造出来的HTTP响应，写入socket中
  //  1)写入首行
  const char* first_line = "HTTP/1.1 200 OK\n";
  send(sock, first_line, strlen(first_line), 0);
  //  2)写入header
  //const char* type_line = "Content-Type: text/html;charset=utf-8\n";
  //const char* type_line = "Content-Type: image/jpg;charset=utf-8\n";   
  //send(sock, type_line, strlen(type_line), 0);
  //  3)写入空行
  const char* blank_line = "\n";
  send(sock, blank_line, strlen(blank_line), 0);
  //  4)写入body(文件内容)
  /*
  ssize_t file_size = GetFileSize(file_path);
  ssize_t i = 0;
  for(; i < file_size; ++i)
  {
    char c;
    read(fd, &c, 1);
    send(sock, &c, 1, 0);
  }
  */
  sendfile(sock, fd , NULL, GetFileSize(file_path));

  //3.关闭文件
  close(fd);
  return 200;
}

int HandlerStaticFile(int sock,Request* req){
  
  //根据url_path获取到文件在服务器上的真是路径
  char file_path[SIZE] = {0};
  HandlerFilePath(req->url_path,file_path);
  //2.读取文件，把文件的内容直接写到socket之中
  int err_code = WriteStaticFile(sock, file_path);

  return err_code;
  
}


int HandlerCGIFather(int new_sock, int father_read, 
    int father_write,int child_pid, Request* req)
{
  //1.如果是POST请求，就把body写入到管道中
  if(strcasecmp(req->method,"POST") == 0)
  {
    int i = 0;
    char c = '\0';
    for(; i < req->content_length; ++i)
    {
      read(new_sock, &c, 1);
      write(father_write, &c, 1);
    }
  }
  //2.构造HTTP响应
  const char* first_line = "HTTP/1.1 200 OK\n";
  send(new_sock, first_line, strlen(first_line), 0);
  const char* type_line= "content-Type: text/html;charset=utf-8\n";
  send(new_sock, type_line, strlen(type_line), 0);
  const char* blank_line = "\n";
  send(new_sock, blank_line, strlen(blank_line), 0);
  //3.循环从管道中读取数据并写入到 socket 
  char c = '\0';
  while(read(father_read, &c, 1) > 0)
  {
    send(new_sock, &c, 1, 0);
  }
  //4.回收子进程的资源
  //wait(NULL);
  waitpid(child_pid, NULL, 0);
  return 200;
}

int HandlerCGIChilr(int child_read, int child_write, 
    Request* req)
{
  //1.设置必要的环境变量
  char method_env[SIZE] = {0};
  sprintf(method_env, "REQUEST_METHOD=%s",req->method);
  putenv(method_env);
  //还要设置 QUERY_STRING 或者 CONTENT_LENGTH
  if(strcasecmp(req->method, "GET") == 0)
  {
    char query_string_env[SIZE] = {0};
    sprintf(query_string_env, "QUERY_STRING=%s", 
        req->query_string);
    putenv(query_string_env);
  }
  else{
    char content_length_env[SIZE] = {0};
    sprintf(content_length_env, "CONTENT_LENGTH=%d",
        req->content_length);
    putenv(content_length_env);
  }
  //2.把标准输入输出重定向到管道中
  dup2(child_read, 0);
  dup2(child_write, 1);
  //3.对子进程进行程序替换
  // url_path: /cgi-bin/test 
  // file_path:./wwwroot/cgi-bin/test 
  char file_path[SIZE] = {0};
  HandlerFilePath(req->url_path, file_path);
  //l 
  //lp
  //le
  //v 
  //vp
  //ve
  execl(file_path, file_path, NULL);
  exit(1);

  return 200;
}
 
int HandlerCGI(int new_sock,Request* req)
{
  int err_code = 200;
  //1.创建一对匿名管道
  int fd1[2],fd2[2];
  pipe(fd1);
  int ret = pipe(fd1);
  if(ret < 0)
    return 404;

  ret = pipe(fd2);
  if(ret < 0)
  {
    close(fd1[0]);
    close(fd1[1]);
    return 404;
  }
  // fd1   fd2描述性差
  // 在此处定义几个明确的变量名进行描述
  int father_read = fd1[0];
  int child_write = fd1[1];
  int father_write = fd2[1];
  int child_read = fd2[0];
  //2.创建子进程
  ret = fork();
  //3.父子进程各自执行不同的逻辑
  if(ret > 0)
  {
    //父进程
    //此处父进程优先关闭这两个管道的文件描述符，
    //是为了后续父进程从子进程这里读取数据的时候，能够读到EOF
    //对于管道来说，所有的写端关闭继续读，才有EOF
    //而此处的所有写端，一方面是父进程需要关闭，另一方面是子进程
    //也需要关闭
    //所以此处父进程先关闭不必要的写端之后，后续子进程用完了
    //直接关闭，父进程也就读到了EOF
    close(child_read);
    close(child_write);
    err_code = HandlerCGIFather(new_sock, father_read, 
        father_write, ret, req);  
  }
  else if(ret == 0)
  {
    //子进程
    close(father_write);
    close(father_read);
    err_code = HandlerCGIChilr(child_read, child_write, req);
  }
  else{
    perror("fork");
    err_code = 404;
    goto END;
  }
  //4.收尾工作和错误处理
END:
  close(fd1[0]);
  close(fd2[0]);        
  close(fd1[1]);                      
  close(fd2[1]); 
  return err_code;
}


//请求处理函数
void HandlerRequest(int new_sock){
	   int err_code = 200;
	   //1.读取并解析请求(反序列化)
	   Request req;
	   memset(&req,0,sizeof(req));
	   //a)从socket中读取出首行
	   if(ReadLine(new_sock,req.first_line,sizeof(req.first_line)) < 0){
		   //TODO 失败处理
		   err_code = 404;
		   goto END;
	   }
	   //b)解析首行，从首行中解析出url和method
	   if(ParseFirstLine(req.first_line,&req.url,&req.method)){
		   //TODO 失败处理
		   err_code = 404;
		   goto END;
	   }
	   //c)解析url,从url之中解析出url_path，query_string
	   if(ParseQueryString(req.url,&req.url_path,&req.query_string)){
	      //TODO 失败处理
		  err_code = 404;
		  goto END;
	   }
	   //d)处理Header，丢弃了大部分header只读取content_length
	   if(ParseHeader(new_sock,&req.content_length)){
		   //TODO 失败处理
		   err_code = 404;
		   goto END;
	   }
	   //2.静态/动态方式生成页面,把生成结果写回到客户端上
	   //假如浏览器发送的请求为"Get"/"get"
	   //strcasecmp()不区分大小写比较
	   if(strcasecmp(req.method,"GET") == 0 && req.query_string == NULL)
     {
		   //a)如果请求时GET请求，并且没有query_string	  
		   //那么就返回静态页面
		  err_code = HandlerStaticFile(new_sock,&req);
	   }
     else if(strcasecmp(req.method,"Get") == 0 && req.query_string != NULL)
     {
		   //b)如果请求是GET请求，并且有query_string
		   //		那么返回动态页面	
		   err_code = HandlerCGI(new_sock, &req);
	   }
     else if(strcasecmp(req.method,"POST") == 0)
     {
		   //c)如果请求是POST请求（一定是蚕食的，参数是通过body来传给服务器），
		   //		那么也返回动态页面
		   err_code = HandlerCGI(new_sock, &req);
     }
     else
     {
		   //TODO 错误处理
		   err_code = 404;
		   goto END;
	   }

     PrintRequest(&req);
	   //错误处理：返回一个404
END:
	   if(err_code != 200){
		   printf("!200");   
       Handler404(new_sock);
	   }
	   close(new_sock);
	   return;
}


void* ThreadEntry(void* arg){
	   int64_t new_sock = (int64_t)arg;
	   //使用HandlerRequest函数进行完成具体的处理请求过程，
	   //这个过程单独取出来为了解耦合
	   //一旦需要把服务器改成多进程或者IO多路复用的形式
	   //整体代码的改动是比较小的
	   HandlerRequest(new_sock);
	   return NULL;
}

//服务器启动
void HttpServerStart(const char* ip,short port){
	int listen_sock =socket(AF_INET,SOCK_STREAM,0);
	if(listen_sock < 0){
		   perror("socket");
		   return;
	}
  //加上这个选项就能重用TIME-WAIT连接
  int opt = 1;
  setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	int ret = bind(listen_sock,(sockaddr*)&addr,sizeof(addr));
	if(ret < 0){
		   perror("bind");
		   return;
  }
	ret = listen(listen_sock,5);
	if(ret < 0){
		   perror("listen");
		   return;
	}
	printf("ServerInit OK\n");
	while(1){
		   sockaddr_in peer;
		   socklen_t len = sizeof(peer);
		   int64_t new_sock = accept(listen_sock,(sockaddr*)&peer,&len);
		   if(new_sock < 0){
			      perror("accept");
				  continue;
		   }
		   //使用多线程的方式来实现TCP服务器
		   pthread_t tid;
		   pthread_create(&tid, NULL, ThreadEntry, (void*)new_sock);
       pthread_detach(tid);
	}
}





//./http_server [ip] [port]
int main(int argc,char* argv[])
{
	if(argc != 3)
  {
		printf("Usage ./http_server [ip] [port]\n");
		return 1;
	}
	HttpServerStart(argv[1],atoi(argv[2]));
  return 0;
}
```
