```html
<html>
  <head>
  <meta http-equiv="content-type" 
  content="text/html;charset=utf-8">
  </head>

  <body>
  <!-- action的含义是使用哪个CGI程序完成动态页面生成 -->
  <form action="/cgi-bin/test" method = "POST">
    First Number:<br> 
    <input type="text" name="a" value="10">
    <br>
    Second Number:<br>
    <input type="text" name="b" value="20">
    <br><br>
    <input type="submit" value="计算">
  </form> 
  </body>
</html>

```
