MAJOR := 1
MINOR := 0
VERSION := $(MAJOR).$(MINOR)

NAME = gs1encoders
APP = $(NAME).bin
APP_STATIC = $(NAME)-linux.bin
TEST_BIN = $(NAME)-test

ifeq ($(SANITIZE),yes)
CC=clang
SAN_CFLAGS = -fsanitize=address,leak,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
SAN_LDFLAGS = -fuse-ld=lld
SAN_ENV = ASAN_OPTIONS="symbolize=1 detect_leaks=1" LSAN_OPTIONS="fast_unwind_on_malloc=0:malloc_context_size=50" ASAN_SYMBOLIZER_PATH="$(shell which llvm-symbolizer)"
endif

CFLAGS = -g -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wconversion -Wformat -Wformat-security -pedantic -Werror -MMD -fPIC $(SAN_CFLAGS)
CFLAGS_TEST = -g -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wformat -Wformat-security -pedantic -Werror -MMD -fPIC $(SAN_CFLAGS)
LDLIBS = -lc
LDFLAGS = -Wl,--as-needed -Wl,-Bsymbolic-functions -Wl,-z,relro -Wl,-z,now $(SAN_LDFLAGS)
LDFLAGS_SO = -shared -Wl,-soname,lib$(NAME).so.$(MAJOR)

PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib

APP_SRC = gs1encoders-app.c
APP_OBJ = $(APP_SRC:.c=.o)

TEST_SRC = gs1encoders-test.c
TEST_OBJ = $(TEST_SRC:.c=.o)

SRCS = $(filter-out $(APP_SRC) $(TEST_SRC), $(wildcard *.c))
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)


.PHONY: all clean app app-static lib libshared libstatic install install-static install-shared uninstall test

all: lib app-static
lib: libshared libstatic
libshared: lib$(NAME).so.$(VERSION) lib$(NAME).so lib$(NAME).so.$(MAJOR)
libstatic: lib$(NAME).a
app: $(APP)
app-static: $(APP_STATIC)


#
#  Shared library
#
lib$(NAME).so: lib$(NAME).so.$(VERSION) lib$(NAME).so.$(MAJOR)
	ln -sf $< $@

lib$(NAME).so.$(MAJOR): lib$(NAME).so.$(VERSION)
	ln -sf $< $@

lib$(NAME).so.$(VERSION): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_SO) $(LDLIBS) $^ -o $@


#
#  Static library
#
lib$(NAME).a: $(OBJS)
	$(AR) cr $@ $^
	ranlib $@


#
#  Application build to use shared library
#
$(APP): lib$(NAME).so $(APP_SRC)
	$(CC) $(CFLAGS) $(APP_SRC) -o $@ -L. -l$(NAME)


#
#  Standalone application
#
$(APP_STATIC): $(OBJS) $(APP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(APP_OBJ) -o $(APP_STATIC)


#
#  Test binary
#
$(TEST_OBJ): $(TEST_SRC)
	$(CC) $(CFLAGS_TEST) -c -o $(TEST_OBJ) $(TEST_SRC)

$(TEST_BIN): $(OBJS) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(TEST_OBJ) -o $(TEST_BIN)


#
#  Utility targets
#
test: $(TEST_BIN)
	$(SAN_ENV) ./$(TEST_BIN) -v

clean:
	$(RM) $(OBJS) $(APP_OBJ) $(APP) $(APP_STATIC) $(TEST_BIN) $(TEST_OBJ) *.so* *.a *.d

install: install-static install-shared

install-headers:
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 0644 $(NAME).h $(DESTDIR)$(PREFIX)/include

install-static: libstatic install-headers
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0644 lib$(NAME).a $(DESTDIR)$(LIBDIR)

install-shared: libshared install-headers
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0644 lib$(NAME).so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR) && ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so
	cd $(DESTDIR)$(LIBDIR) && ln -sf lib$(NAME).so.$(VERSION) lib$(NAME).so.$(MAJOR)
	-ldconfig

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/include/$(NAME).h
	$(RM) $(DESTDIR)$(PREFIX)/lib/lib$(NAME).so.$(VERSION)
	$(RM) $(DESTDIR)$(PREFIX)/lib/lib$(NAME).so.$(MAJOR)
	$(RM) $(DESTDIR)$(PREFIX)/lib/lib$(NAME).so
	$(RM) $(DESTDIR)$(PREFIX)/lib/lib$(NAME).a
	-ldconfig


-include $(DEPS)
