CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET   := socks5_proxy
SRCS     := main.cpp server.cpp session.cpp resolver.cpp epoll.cpp socks5.cpp dns.cpp credentials.cpp

$(TARGET): $(SRCS) $(wildcard *.hpp)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: clean