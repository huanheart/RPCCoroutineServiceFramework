# 编译器和编译选项
CXX = g++
CXXFLAGS = -g -O0  -I/usr/local/muduo/include

# 库路径
LIBS = -lprotobuf  -lssl -lcrypto -lpthread -lmuduo_base
#RPC/rpcheader.pb.h.o
# 源文件和目标文件
objects = main.o  CoroutineLibrary/fd_manager.o  CoroutineLibrary/fiber.o CoroutineLibrary/hook.o   CoroutineLibrary/timer.o  CoroutineLibrary/scheduler.o  CoroutineLibrary/ioscheduler.o  CoroutineLibrary/thread.o  server/tcp_server.o  rpc_server.o  server/rpc_conn.o  util/config.o   util/address.o    util/socket.o  RPC/rpcheader.pb.o

# 最终目标文件
edit: $(objects)
	$(CXX) $(CXXFLAGS) -o CoroutineServer $(objects) $(LIBS)

main.o :
	$(CXX) $(CXXFLAGS) -c main.cpp


fd_manager.o : CoroutineLibrary/thread.h
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/fd_manager.cpp

fiber.o :
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/fiber.cpp

hook.o :
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/hook.cpp

scheduler.o : CoroutineLibrary/hook.h  CoroutineLibrary/fiber.h CoroutineLibrary/thread.h
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/scheduler.cpp

timer.o :
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/timer.cpp

ioscheduler.o : CoroutineLibrary/timer.h CoroutineLibrary/scheduler.h CoroutineLibrary/ioscheduler.h
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/ioscheduler.cpp

config.o :
	$(CXX) $(CXXFLAGS) -c util/config.cpp

thread.o :
	$(CXX) $(CXXFLAGS) -c CoroutineLibrary/thread.cpp
tcp_server.o :    server/tcp_server.h  util/socket.h  util/noncopyable.h  CoroutineLibrary/ioscheduler.h
	$(CXX) $(CXXFLAGS) -c server/tcp_server.cc


rpc_server.o : server/tcp_server.h util/socket.h
	$(CXX) $(CXXFLAGS) -c server/rpc_server.cpp

rpc_conn.o :   RPC/rpcheader.pb.h  server/rpc_server.h
	$(CXX) $(CXXFLAGS) -c server/rpc_conn.cpp

address.o : util/endian.h
	$(CXX) $(CXXFLAGS) -c util/address.cc


socket.o : util/address.h  util/noncopyable.h
	$(CXX) $(CXXFLAGS) -c util/socket.cc

rpcheader.pb.o :
	$(CXX) $(CXXFLAGS) -c RPC/rpcheader.pb.cc


# 清理目标文件
.PHONY: clean
clean:
	rm -f CoroutineServer $(objects)


