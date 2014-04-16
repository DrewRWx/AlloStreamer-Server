UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
LIBS=\
-L/opt/local/lib \
$(addprefix -L,$(wildcard /opt/local/lib/live/*)) \
-lboost_thread-mt \
-lboost_system-mt

INCLUDE=\
-I/usr/local/include \
-I/opt/local/include \
$(addprefix -I,$(wildcard /opt/local/lib/live/*/include))
else
LIBS=\
-lboost_thread \
-lboost_system \
-lrt

INCLUDE=\
-I/usr/include/liveMedia \
-I/usr/include/groupsock \
-I/usr/include/UsageEnvironment \
-I/usr/include/BasicUsageEnvironment

EXTRA_FLAGS= -Wl,--no-as-needed
endif

LIBS+=\
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
-lm

SRC=\
MyDeviceSource.cpp \
AlloServer.c \
serverUni.cpp \
H264VideoOnDemandServerMediaSubsession.cpp

all: clean
	g++ -O2 -o AlloServer -g $(EXTRA_FLAGS) $(SRC) $(LIBS) $(INCLUDE)

clean:
	rm AlloServer
