.PHONY: clean

JAVA_HOME=/usr/local/jdk-10.0.1
TARGET = libjnativetracer.so
OBJS = jnativetracer.o
CXX = g++
CXXFLAGS = -fPIC -g
CPPFLAGS = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
LDFLAGS = -lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
