.PHONY: clean

TARGET = libjnativetracer.so
OBJS = jnativetracer.o
CXX ?= g++
CXXFLAGS = -fPIC -g -std=c++11
CPPFLAGS = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
LDFLAGS = -lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
