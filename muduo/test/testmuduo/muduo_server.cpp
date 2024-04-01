/*
muduo网络库给用户提供了两个主要的类
TcpServer ： 用于编写服务器程序的
TcpClient ： 用于编写客户端程序的

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
                        用户的连接和断开       用户的可读写事件
*/
/*
这个test文件不属于项目代码，只是为了分别测试json、cmake原理、muduo库的使用
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写时间的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/
//下面代码写法十分固定，除了类名ChatServer和ip地址端口号可以不同，其他都一样写
//这就是使用muduo库的方式
class ChatServer
{   //创建ChatServer需要三个参数
public:
    ChatServer(EventLoop *loop,               // 事件循环（epoll），下面有定义
               const InetAddress &listenAddr, // IP+Port
               const string &nameArg)//服务器名称
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 当服务器监听到用户连接的创建和断开，就回调函数onConnection，_1是参数占位符
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册已连接用户读写事件绑定回调函数onMessage
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量 1个I/O线程   3个worker线程（工作线程）
        _server.setThreadNum(4);
    }
    //开始事件循环（监听或者读写）
    void start()
    {
        _server.start();
    }

private://下面的函数都在TCPsever.h文件中有，函数名字修改了，原名字在public里有，但参数不变
    // 专门处理用户的连接创建和断开  epoll listenfd accept
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {   //连接成功就打印对应信息，peerAddress是结构体，封装在TcpConnection.h中
            //结构体里封装了ip和端口
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << endl;
        }
        else
        {   //失败就关闭连接
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << endl;
            conn->shutdown(); // close(fd)
            // _loop->quit();或者这个，lfd离开epoll
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接，通过这个连接可以发送数据
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 接收到数据的时间信息
    {   //buf接收数据
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toFormattedString() << endl;
        conn->send(buf);
    }

    TcpServer _server; // #1，就是TCP服务器对象
    EventLoop *_loop;  // #2 epoll，事件循环的指针
};

int main()
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl=>epoll，lfd上树
    loop.loop();    // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
}