CXXFLAGS = -W -Wall

all: replay_remote

replay_remote_OBJS = \
	replay_remote.o \
	frame_buffer.o \
	mjpeg_camera.o \
	mjpeg_frame.o \
	thread.o \
	condition.o \
	mutex.o

replay_remote: $(replay_remote_OBJS)
	$(CXX) $(LDFLAGS) -g -o $@ $^ -pthread

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -g -c -o $@ $^

clean:
	rm -f replay_remote *.o
