#include<stdio.h>
#include<time.h>
#include<fcntl.h>
#include<mysql/mysql.h>
#include"cgi_base.h"


void RenderHtml(struct tm* tp,char * row)
{
  // 1.打开 detail.html 文件
  int fd = open("./wwwroot/worldtime/detail.html", O_RDONLY);
  if(fd < 0)
  {
    fprintf(stderr,"detail.html 打开失败\n");
    return;
  }
  // 2.逐个字节读取文件中的内容
  char c = '\0';
  while(read(fd, &c, 1) > 0)
  {
    // 3.判定当前字符位置是否需要替换成时间信息
    if(c == '\1')
    {
      // 这个位置需要替换成时间
      printf("%s--",row);
      printf("%d年%d月%d日-%02d:%02d", tp->tm_year+1900
          ,tp->tm_mon+1,tp->tm_mday,(int)tp->tm_hour,tp->tm_min);
      // printf("%s",asctime(tp));
      fflush(stdout);
    }
    else{
      // 4.读到的内容同时写到标准输出上
      write(1, &c, 1);
    }
  }
  // 5.关闭文件
  close(fd);
}


int main()
{
  // 1.获取到 query_string
  char query_string[1024 * 4] = {0};
  int ret = GetQueryString(query_string);
  if(ret < 0)
  {
    fprintf(stderr, "GetQueryString failed\n");
    return 1;
  }
  // query_string 形如 city=%e5%8c%97%e4%ba%ac
  char city[1024 * 4] = {0};
  sscanf(query_string, "city=%s", city);

  // 2.从数据库中获取时差（完整的数据访问操作）
  //a.连接到数据库
  //有了一个空遥控器
  MYSQL* connect_fd = mysql_init(NULL);
  //拿遥控器进行配对
  MYSQL* connect_ret = mysql_real_connect(connect_fd, "127.0.0.1"
      ,"root", "1234", "http", 3306, NULL, 0);
  if(connect_ret == NULL)
  {
    fprintf(stderr, "mysql connect failed\n");
    return 1;
  }
  fprintf(stderr,"mysql connect ok!\n");

  //b.拼装sql语句
  //组织命令
  char sql[1024 * 4] = {0};
  sprintf(sql, 
      "select timediff,name from WorldTime where city='%s'"
      ,city);

  //c.把sql语句发到服务器上
  //使用遥控器把命令发给服务器
  ret = mysql_query(connect_fd, sql);
  if(ret < 0)
  {
    fprintf(stderr, "mysql_query failed! %s\n", sql);
    mysql_close(connect_fd);
    return 1;
  }

  //d.读取并遍历服务器返回的结果
  MYSQL_RES* result = mysql_store_result(connect_fd);
  if(result == NULL)
  {
    fprintf(stderr, "mysql_store_result failed!\n");
    return 1;
  }
  // c) 获取到每个元素的具体值
  // 只有一行数据
  MYSQL_ROW row = mysql_fetch_row(result);
  if(row == NULL)
  {
    fprintf(stderr, "mysql 查询失败：city 不存在！\n");
    mysql_close(connect_fd);
    return 1;
  }
  mysql_close(connect_fd);
  double time_diff = atof(row[0]);
  // 3.获取系统时间，根据时差进行进行计算，得到对方城市的时间
  // 当前时间戳
  time_t t = time(NULL);
  t += time_diff * 3600;
  struct tm* tp = localtime(&t);
  // printf("<html>");
  // printf("%s--",row[1]);
  // printf("%d年%d月%d日-%02d:%02d", tp->tm_year+1900,tp->tm_mon+1
  //     ,tp->tm_mday,(int)tp->tm_hour,tp->tm_min);
  // printf("%s",asctime(tp));
  // printf("</html>");
  RenderHtml(tp,row[1]);
  return 1;
}
