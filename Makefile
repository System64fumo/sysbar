EXEC = sysbar
PKGS = gtkmm-4.0 gtk4-layer-shell-0 libcurl wireplumber-0.5 dbus-1
SRCS +=	$(wildcard src/*.cpp)
SRCS +=	$(wildcard src/modules/*.cpp)
OBJS = $(patsubst src/%,build/%,$(patsubst src/modules/%,build/%,$(SRCS:.cpp=.o)))

DESTDIR = $(HOME)/.local
BUILDDIR = build

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
