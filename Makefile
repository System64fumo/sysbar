BINS = sysbar
LIBS = libsysbar.so
PKGS = gtkmm-4.0 gtk4-layer-shell-0
SRCS = $(wildcard src/*.cpp)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
DATADIR ?= $(PREFIX)/share
BUILDDIR = build

# Include enabled modules from config.hpp
ifneq (, $(shell grep -E '^#define MODULE_CLOCK' src/config.hpp))
	SRCS += src/modules/clock.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_WEATHER' src/config.hpp))
	SRCS += src/modules/weather.cpp
	PKGS += libcurl jsoncpp
endif
ifneq (, $(shell grep -E '^#define MODULE_TRAY' src/config.hpp))
	SRCS += src/modules/tray.cpp
	PKGS += dbus-1
endif
ifneq (, $(shell grep -E '^#define MODULE_HYPRLAND' src/config.hpp))
	SRCS += src/modules/hyprland.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_VOLUME' src/config.hpp))
	SRCS += src/modules/volume.cpp
	PKGS += wireplumber-0.5
else
	SRCS := $(filter-out src/wireplumber.cpp, $(SRCS))
endif
ifneq (, $(shell grep -E '^#define MODULE_NETWORK' src/config.hpp))
	SRCS += src/modules/network.cpp
	CXXFLAGS += -I /usr/include/libnl3/ -lnl-3 -lnl-genl-3
endif
ifneq (, $(shell grep -E '^#define MODULE_BATTERY' src/config.hpp))
	SRCS += src/modules/battery.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_NOTIFICATION' src/config.hpp))
	SRCS += src/modules/notifications.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_PERFORMANCE' src/config.hpp))
	SRCS += src/modules/performance.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_TASKBAR' src/config.hpp))
	SRCS += src/modules/taskbar.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_BACKLIGHT' src/config.hpp))
	SRCS += src/modules/backlight.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_MPRIS' src/config.hpp))
	SRCS += src/modules/mpris.cpp
	PKGS += playerctl libcurl
endif
ifneq (, $(shell grep -E '^#define MODULE_BLUETOOTH' src/config.hpp))
	SRCS += src/modules/bluetooth.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_CONTROLS' src/config.hpp))
	SRCS += src/modules/controls.cpp
endif
ifneq (, $(shell grep -E '^#define MODULE_CELLULAR' src/config.hpp))
	SRCS += src/modules/cellular.cpp
endif
ifeq (, $(shell grep -E '^#define FEATURE_WIRELESS' src/config.hpp))
	SRCS := $(filter-out src/wireless_network.cpp, $(SRCS))
endif

VPATH = src src/modules
OBJS = $(patsubst src/%, $(BUILDDIR)/%, $(patsubst src/modules/%, $(BUILDDIR)/%, $(SRCS:.cpp=.o)))

CFLAGS += -Oz -s -flto -fPIC -fomit-frame-pointer
CXXFLAGS += $(CFLAGS)
LDFLAGS += -Wl,--as-needed,-z,now,-z,pack-relative-relocs

CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

PROTOS = $(wildcard proto/*.xml)
PROTO_HDRS = $(patsubst proto/%.xml, src/%.h, $(PROTOS))
PROTO_SRCS = $(patsubst proto/%.xml, src/%.c, $(PROTOS))
PROTO_OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(PROTO_SRCS:.c=.o))

$(shell mkdir -p $(BUILDDIR))
JOB_COUNT := $(BINS) $(LIBS) $(PROTO_HDRS) $(PROTO_SRCS) $(PROTO_OBJS) $(OBJS) src/git_info.hpp
JOBS_DONE := $(shell ls -l $(JOB_COUNT) 2> /dev/null | wc -l)

define progress
	$(eval JOBS_DONE := $(shell echo $$(($(JOBS_DONE) + 1))))
	@printf "[$(JOBS_DONE)/$(shell echo $(JOB_COUNT) | wc -w)] %s %s\n" $(1) $(2)
endef

all: $(BINS) $(LIBS)

install: $(BINS)
	@echo "Installing..."
	@install -D -t $(DESTDIR)$(BINDIR) $(BUILDDIR)/$(BINS)
	@install -D -t $(DESTDIR)$(LIBDIR) $(BUILDDIR)/$(LIBS)
	@install -D -t $(DESTDIR)$(DATADIR)/sys64/bar config.conf style.css events.css calendar.conf

clean:
	@echo "Cleaning up"
	@rm -r $(BUILDDIR) src/git_info.hpp $(PROTO_HDRS) $(PROTO_SRCS)

uninstall:
	@echo "Uninstalling..."
	@rm -f $(DESTDIR)$(BINDIR)/$(BINS)
	@rm -f $(DESTDIR)$(LIBDIR)/$(LIBS)

$(BINS): src/git_info.hpp $(BUILDDIR)/main.o $(BUILDDIR)/config_parser.o
	$(call progress, Linking $@)
	@$(CXX) -o \
	$(BUILDDIR)/$(BINS) \
	$(BUILDDIR)/main.o \
	$(BUILDDIR)/config_parser.o \
	$(CXXFLAGS) \
	-lgtkmm-4.0 -lglibmm-2.68 -lgiomm-2.68 -lgtk4-layer-shell

$(LIBS): $(PROTO_HDRS) $(PROTO_SRCS) $(PROTO_OBJS) $(OBJS)
	$(call progress, Linking $@)
	@$(CXX) -o \
	$(BUILDDIR)/$(LIBS) \
	$(filter-out $(BUILDDIR)/main.o, $(OBJS)) \
	$(PROTO_OBJS) \
	$(CXXFLAGS) \
	$(LDFLAGS) \
	-shared

$(BUILDDIR)/%.o: %.cpp
	$(call progress, Compiling $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/%.o: %.c
	$(call progress, Compiling $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(PROTO_HDRS): src/%.h : proto/%.xml
	$(call progress, Creating $@)
	@wayland-scanner client-header $< $@

$(PROTO_SRCS): src/%.c : proto/%.xml
	$(call progress, Creating $@)
	@wayland-scanner public-code $< $@

src/git_info.hpp:
	$(call progress, Creating $@)
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format="%s" $$commit_hash | sed 's/"/\\\"/g'); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp
