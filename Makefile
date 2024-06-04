EXEC = sysbar
PKGS = gtkmm-4.0 gtk4-layer-shell-0
SRCS +=	$(wildcard src/*.cpp)
SRCS +=	$(wildcard src/modules/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DESTDIR = $(HOME)/.local

CXXFLAGS += -march=native -mtune=native -Os -s -Wall -flto=auto -fno-exceptions
CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

$(EXEC): $(OBJS)
	$(CXX) -o $(EXEC) $(OBJS) \
	$(LDFLAGS) \
	$(CXXFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ \
	$(CXXFLAGS)

install: $(EXEC)
	mkdir -p $(DESTDIR)/bin
	install $(EXEC) $(DESTDIR)/bin/$(EXEC)

clean:
	rm $(EXEC) $(SRCS:.cpp=.o)
