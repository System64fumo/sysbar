EXEC = sysbar
PKGS = gtkmm-4.0 gtk4-layer-shell-0
SRCS +=	$(wildcard src/*.cpp)

DESTDIR = $(HOME)/.local
BUILDDIR = build

# Include enabled modules from config.hpp
ifneq (, $(shell grep -E '^#define MODULE_CLOCK' src/config.hpp))
	SRCS += src/modules/clock.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_WEATHER' src/config.hpp))
	SRCS += src/modules/weather.cpp
	PKGS += libcurl
endif
ifneq (, $(shell grep -E '^#define MODULE_TRAY' src/config.hpp))
	SRCS += src/modules/tray.cpp
	PKGS += dbus-1
endif
ifneq (, $(shell grep -E '^#define MODULE_VOLUME' src/config.hpp))
	SRCS += src/modules/volume.cpp
	PKGS += wireplumber-0.5
else
	SRCS := $(filter-out src/wireplumber.cpp, $(SRCS))
endif
ifneq (, $(shell grep -E '^#define MODULE_NETWORK' src/config.hpp))
	SRCS += src/modules/network.cpp
endif

OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(patsubst src/modules/%,$(BUILDDIR)/%,$(SRCS:.cpp=.o)))

CXXFLAGS += -march=native -mtune=native -Os -s -Wall -flto=auto
CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

$(shell mkdir -p $(BUILDDIR))

$(EXEC): src/git_info.hpp $(OBJS)
	$(CXX) -o \
	$(BUILDDIR)/$(EXEC) \
	$(OBJS) \
	$(LDFLAGS) \
	$(CXXFLAGS)

$(BUILDDIR)/%.o: src/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/%.o: src/modules/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

src/git_info.hpp:
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format=%s $$commit_hash); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp

install: $(EXEC)
	mkdir -p $(DESTDIR)/bin
	install $(BUILDDIR)/$(EXEC) $(DESTDIR)/bin/$(EXEC)

clean:
	rm -r $(BUILDDIR) src/git_info.hpp
