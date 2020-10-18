project(P2P_file_share CXX)
cmake_minimum_required(VERSION 3.10)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -pthread" )
set (TRACKER_SRC src/tcpTracker deprecated/tools/test_client.cpp deprecated/Tracker/connection.h deprecated/Tracker/peers.h deprecated/Tracker/network.h src/tcpClient/TcpClientMain.cpp)

file(GLOB TCPNODE_SRC
        "src/tcpNode/*.h"
        "src/tcpNode/*.cpp"
        )

file(GLOB TCPTRACKER_SRC
        "src/tcpTracker/*.h"
        "src/tcpTracker/*.cpp"
        )

file(GLOB TCPSERVER_SRC
        "src/tcpServer/*.h"
        "src/tcpServer/*.cpp"
        )
file(GLOB TCPCLIENT_SRC
        "src/tcpClient/*.h"
        "src/tcpClient/*.cpp"
        )
file(GLOB COMMON
        "src/common/*.h"
        "src/common/*.cpp"
        )


add_executable(tcpServer ${TCPSERVER_SRC} ${COMMON})
add_executable(tcpClient ${TCPCLIENT_SRC} ${COMMON})
add_executable(tcpTracker ${TCPTRACKER_SRC} ${COMMON})
add_executable(tcpNode ${TCPNODE_SRC} ${COMMON})