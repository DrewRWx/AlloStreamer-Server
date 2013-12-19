deps= ../deps

LIBS=\
-L$(deps)/boost_1_54/build/lib \
-lboost_thread \
-lboost_system \
-lx264 \
-lavcodec \
-lavdevice \
-lavfilter \
-lavformat \
-lavutil \
-lswscale \
-lliveMedia \
-lgroupsock \
-lBasicUsageEnvironment \
-lUsageEnvironment \
-lpthread \
-lm \
-lrt

SRC=\
MyDeviceSource.cpp \
AlloServer.c \
serverUni.cpp \
H264VideoOnDemandServerMediaSubsession.cpp

INCLUDE=\
-I$(deps)/boost_1_54/build/include \
-I/usr/include/liveMedia \
-I/usr/include/groupsock \
-I/usr/include/UsageEnvironment \
-I/usr/include/BasicUsageEnvironment



all:
	g++ -o AlloServer -g -Wl,--no-as-needed $(SRC) $(LIBS) $(INCLUDE) 
