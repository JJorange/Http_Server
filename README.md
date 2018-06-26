```c
```c
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
typedef struct sockaddr sockaddr ;
typedef struct sockaddr_in sockaddr_in;

//一次从socket中读取一行数据
//把数据放到buf缓冲区中
//如果读取失败，返回值为-1
int ReadLine(int sock,char buf[]，ssize_t size){
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
					 return -1；
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
		   pch = strtok_r(input,split_char,&tmp);
	}
	return i;
}

int ParseFirstLine(char fisr_line[],char** p_url,char** p_methon){
	//把首行按照空格进行字符串切分
	char* tok[10];
	//把first_line按照空格进行字符串切分
	//切分得到的每一个部分，就放到tok数组中
	//返回值，就是tok数组中包含几个元素
	//最有一个参数10表示tok数组最多能放几个元素
	int tok_size = Splist(first_line," ",tok,10);
	if(tok_size != 3){
		printf("Split failed! tok_size = %d\n",tok_size)
		   return -1;
	}
	*p_method  = tok[10];
	*p_url = tok[1];
	return 0;
}

int ParseQueryString(char* url,char** p_url_path,char** p_query_string){
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
		   dd;
		   //1.循环从socket中读取一行
		   ssize_t read_size = ReadLine(sock,buf,sizeof(buf));
		   //处理读失败的情况
		   if(read_size <= 0){    
			   return -1;
		   }
		   //处理读完的情况
		   if(strcmp(buf,"\n") == 0){
			      return 0;
		   }
		   //2.判定当前行是不是content_length
		   //  如果是content_length就直接把value读取出来
		   //  如果不是就直接丢弃
		   const char* content_length_str = "Content-Length: "
		   if(content_length != NULL 
				   && strncmp(buf,"Content-Length: ",
					   strlen(content_length_str)) == 0){
			      *conten_length = atoi(buf + strlen(concent_length_str));
		   }
	   }
	   //3.如果是，直接把value读出来
	   //4.如果不是就直接丢弃
	   //5.读到空行，循环结束
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
	   if(strcasecmp(req.method,"GET") == 0 && req.query_string == NULL){
		   //a)如果请求时GET请求，并且没有query_string	  
		   //那么就返回静态页面
		   ret = HandlerStaticFile();
	   }else if(strcasecmp(req.method,"Get") == 0 && req.query_string != NULL){
		   //b)如果请求是GET请求，并且有query_string
		   //		那么返回动态页面	
		   ret = HandlerCGI();
	   }else if(strcasecmp(req.method,"POST") == 0){
		   //c)如果请求是POST请求（一定是有蚕食的，参数是通过body来传给服务器），
		   //		那么也返回动态页面
		   ret = HandlerCGI();
	   }else{
		   //TODO 错误处理
		   err_code = 404;
		   goto END;
	   }
	   //错误处理：返回一个404
END:
	   if(err_code != 200){
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
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	int ret = bind(listen_sock,(sockaddr*)&addr.sizeof(addr));
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
		   pthread_create(&tid,NULL)
	}
}














//./http_server [ip] [port]
int main(int argc,char* argv[])
{
	if(argc != 3){
		printf("Usage ./http_server [ip] [port]\n");
		return 1;
	}
	HttpServerStart(argv[1],atoi(argv[2]));
	   return 0;
}

```

```
