
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

// 分GET、POST两种情况读取计算的参数  
// 1.GET 从query_string读取  
// 2.POST 从body中读取
// 读取的结果放在buf输出缓冲区中
static int GetQueryString(char buf[])
{
  
  //1.先从环境变量中获取到方法是什么
  char* method = getenv("REQUEST_METHOD");
  if(method == NULL)
  {
    //由于当前的CGI程序对应的标准输出已经被重定向到管道上了
    //而这部分数据又会被返回给客户端
    //避免让程序内部的错误暴露给普通用户
    //通过 stderr 作为输出日志的手段
    fprintf(stderr, "[CGI] error == NULL\n");
    return -1;
  }
  //2.判定方法时GET还是POST
  //如果是GET，就是环境变量里面读取 QUERY_STRING
  //如果是POST，就需要从环境变量里面读取 CONTENT_LENGTH
  if(strcasecmp(method, "GET") == 0)
  {
    char* query_string = getenv("QUERY_STRING");
    if(query_string == NULL)
    {
      fprintf(stderr, "[CGI] query_string is NULL\n");
      return -1;
    }
    //拷贝完成后，buf里面的内容形如
    //a=10&b=20
    strcpy(buf, query_string);
  }
  else{
    char* content_length_str = getenv("CONTENT_LENGTH");
    if(content_length_str == NULL)
    {
      fprintf(stderr, "[CGI] content_length is NULL\n");
      return -1;
    }
    int content_length = atoi(content_length_str);
    int i = 0;
    for(; i < content_length; ++i)
    {
      //此处由于父进程把body已经写入管道
      //子进程又已经把0号文件描述符重定向到了管道
      //此时从标准输入来读数据，也就读到了管道中的数据
      read(0, &buf[i], 1);
    }
    //循环读完之后，buf里面的内容形如
    //a=10&b=20
  }
  return 0;
}
