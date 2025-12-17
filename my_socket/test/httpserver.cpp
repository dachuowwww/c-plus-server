#include "HttpServer.h"
#include <unistd.h>
#include <iostream>
#include <memory>
#include "Connection.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "TimeStamp.h"

void Message(const std::shared_ptr<Connection> &conn) {  // 注册回调函数,需要修改内部元素所以不能设为const
  // conn->Read();
  std::cout << "new message from client " << conn->GetFd() << " : " << conn->ReadInputBuffer() << std::endl;
  conn->Send(conn->ReadInputBuffer());
}

void Http(const HttpRequest &request, HttpResponse *response) {
  if (request.GetMethodString() != "GET") {
    response->SetStatusCode(HttpResponse::HttpStatusCode::BAD_RESQUEST);
    response->SetStatusMessage("BAD_RESQUEST");
    response->SetClose();
  } else {
    if (request.GetURL() == "/select") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Title</title>
    <style>
        .box{
            background-color: lightgoldenrodyellow;
            position: relative;
            width: 200px;
            height: 200px;
            margin: 100px auto;
            padding-left: 50px;
            padding-right:50px;
            border: 2px solid #a5a5a5;
            border-radius: 30px 0px 30px 0px;
            box-shadow: 2px 2px 2px #e1e5ee;
        }
        h2{
            width: 100px;
            height: 24px;
            font-size: 24px;
        }
        span{
            color: darkred;
        }
        button {
            float: left;
            width: 60px;
            margin:50px 20px 20px 20px;
        }

    </style>
</head>
<body>
<div class="box">
    <h2>随机点名</h2>
    <p>抽到的人是：<span>xxx</span></p>
    <button class = "start">开始</button>
    <button class = 'over'>结束</button>
</div>
<script>
    let names =['谢可欣','杨宜铨','yyq','qq','欣','宝宝']
    let n
    let random
    const name = document.querySelector('span')
    const start = document.querySelector('.start')
    const over = document.querySelector('.over')
    start. addEventListener('click', function(){
        n = setInterval(function (){
            random = Math.floor(Math.random()*names.length)
            name.innerText = `${names[random]}`
        }, 50)
    })
    over.addEventListener('click',function (){
        clearInterval(n)
        names.splice(random,1)
        //如果放结束按钮外面则点击结束按钮不会触发
        if(names.length === 1){
            start.disabled = over.disabled = true
        }
    })

</script>
</body>
</html>)HTML");
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else if (request.GetURL() == "/") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody("<font color=\"red\">您好!</font>");
    } else if (request.GetURL() == "/helloyyq") {
      response->SetContentType("text/plain; charset=UTF-8");
      response->SetResponseBody("Hello 谢可欣宝宝!\n");
      response->SetStatusCode(HttpResponse::HttpStatusCode::OK);
      response->SetStatusMessage("OK");
    } else {
      response->SetContentType("text/plain");
      response->SetResponseBody("Sorry,not found.\n");
      response->SetStatusCode(HttpResponse::HttpStatusCode::NOT_FOUND);
      response->SetStatusMessage("NOT_FOUND");
      response->SetClose();
    }
  }
}

void Every() { std::cout << TimeStamp::Now().ToFormattedString() << std::endl; }
int main(int argc, char *argv[]) {
  std::string ip = "127.0.0.1";
  int port = 1234;
  if (argc == 3) {
    ip = argv[1];
    port = std::atoi(argv[2]);
  }

  auto httpserver = std::make_unique<HttpServer>(ip.c_str(), port);
  if (argc == 2) { // 加一个参数变echo_server
    httpserver->SetMessageCallBack(Message);
  } else {
    httpserver->SetHttpResponseCallBack(Http);
  }
  httpserver->OnTimerEvery(3.0, Every);
  httpserver->Start();
  sleep(10000);
  return 0;
}
