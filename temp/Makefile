server:
	g++ -std=c++20 -pthread error.cpp Client.cpp Buffer.cpp ThreadPool.cpp -o client && \
	g++ -std=c++20 -pthread error.cpp main.cpp Epoll.cpp InetAddress.cpp Socket.cpp Channel.cpp EventLoop.cpp Server.cpp Acceptor.cpp Connection.cpp Buffer.cpp ThreadPool.cpp -o server

test:
	g++ -std=c++20 -pthread error.cpp main.cpp Epoll.cpp InetAddress.cpp Socket.cpp Channel.cpp EventLoop.cpp Server.cpp Acceptor.cpp Connection.cpp Buffer.cpp ThreadPool.cpp -o server && \
	g++ -std=c++20 -pthread error.cpp Test.cpp InetAddress.cpp Socket.cpp Buffer.cpp ThreadPool.cpp -o test

test_threadpool:
	g++ -std=c++20 -pthread TestThreadPool.cpp ThreadPool.cpp -o test_threadpool

client:
	g++ -std=c++20 -pthread error.cpp Client.cpp Buffer.cpp ThreadPool.cpp -o client

clean_test:
	rm server && rm test

clean_threadpool:
	rm test_threadpool
clean:
	rm server && rm client
