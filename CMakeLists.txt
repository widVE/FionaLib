cmake_minimum_required(VERSION 2.6.2)
project(FionaLib)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(./include)

#OPTION(LINUX_BUILD "Compile for Linux, not Windows" ON)

#if(WIN32)
#	SET(APPSPECIFICSOURCE FionaUTWin32.cpp)
#else()
SET(APPSPECIFICSOURCE FionaUTLinux.cpp)
add_definitions(-DLINUX_BUILD)
add_definitions(-DROSS_TEST)
add_definitions(-DFOR_OGRE)
add_definitions(-DDSCVR)
#endif()

#SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./lib)
ADD_LIBRARY(FionaLib STATIC ${APPSPECIFICSOURCE} FionaUT.cpp FionaScene.cpp FionaUTConfig.cpp FionaUTEvent.cpp FionaNetwork.cpp FionaUTVRPN.cpp Socket.cpp VRAction.cpp FionaUTRender.cpp )
TARGET_LINK_LIBRARIES(FionaLib ./linux/lib/GL ./linux/lib/GLU ./linux/lib/glut ./linux/lib/libvrpn.a ./linux/lib/libvrpnatmel.a ./linux/lib/libvrpnserver.a ) 
#ADD_SUBDIRECTORY("../FionaOgre")
