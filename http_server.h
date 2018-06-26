```c
```c
#pragma once

#define SIZE 10240

typedef struct Request(){
	   char first_line[SIZE];
	   char* method;
	   char* url;
	   char* url_path;//重点关注 一
	   char* query_string;//重点关注 二
	   //char* version;
	   //接下来是header部分，如果要完整的解析下来，
	   //需要使用二叉搜索时或者hash表。
	   //偷个懒，其他header不要了，只保存一个Content_Length
	   int content_length;

}Request;

```

```
