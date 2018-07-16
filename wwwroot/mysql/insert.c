```c
#include<stdio.h>
#include<mysql/mysql/udf_registration_types.h>
#include<mysql/mysql.h>                                            
#include"cgi_base.h" 

int main()
{
  //0.获取 query_string
  char buf[1024 * 4] = {0};                    
  if(GetQueryString(buf) < 0)                
  {                             
    fprintf(stderr, "GetQueryString failed\n");
    return 1;                  
  }

  //
}

```
