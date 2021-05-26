MAJOR := 1
MINOR := 0
VERSION := $(MAJOR).$(MINOR)

NAME = gs1encoders

BUILD_DIR = build


ifeq ($(MAKECMDGOALS),test)
BUILD_DIR = test
UNIT_TEST_CFLAGS = -DUNIT_TESTS
endif


ifeq ($(DEBUG),yes)
DEBUG_CFLAGS = -DPRNT
endif


ifeq ($(MAKECMDGOALS),clean-test)
BUILD_DIR = test
endif

ifeq ($(SANITIZE),yes)
CC=clang
SAN_CFLAGS = -fsanitize=address,leak,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
SAN_LDFLAGS = -fuse-ld=lld
SAN_ENV = ASAN_OPTIONS="symbolize=1 detect_leaks=1" LSAN_OPTIONS="fast_unwind_on_malloc=0:malloc_context_size=50" ASAN_SYMBOLIZER_PATH="$(shell which llvm-symbolizer)"
endif


CFLAGS = -g -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wconversion -Wformat -Wformat-security -pedantic -Werror -MMD -fPIC $(SAN_CFLAGS) $(UNIT_TEST_CFLAGS) $(DEBUG_CFLAGS)
LDLIBS = -lc
LDFLAGS = -Wl,--as-needed -Wl,-Bsymbolic-functions -Wl,-z,relro -Wl,-z,now $(SAN_LDFLAGS)
LDFLAGS_SO = -shared -Wl,-soname,lib$(NAME).so.$(MAJOR)

APP = $(BUILD_DIR)/$(NAME).bin
APP_STATIC = $(BUILD_DIR)/$(NAME)-linux.bin
TEST_BIN = $(BUILD_DIR)/$(NAME)-test
LIB_STATIC = $(BUILD_DIR)/lib$(NAME).a
LIB_SHARED = $(BUILD_DIR)/lib$(NAME).so.$(VERSION) $(BUILD_DIR)/lib$(NAME).so $(BUILD_DIR)/lib$(NAME).so.$(MAJOR)

PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib

APP_SRC = gs1encoders-app.c
APP_OBJ = $(BUILD_DIR)/$(APP_SRC:.c=.o)

TEST_SRC = gs1encoders-test.c
TEST_OBJ = $(BUILD_DIR)/$(TEST_SRC:.c=.o)

ALL_SRCS = $(wildcard *.c)
SRCS = $(filter-out $(APP_SRC) $(TEST_SRC), $(ALL_SRCS))
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))
DEPS = $(addprefix $(BUILD_DIR)/, $(ALL_SRCS:.c=.d))


.PHONY: all clean app app-static lib libshared libstatic install install-static install-shared uninstall test clean-test

default: lib app-static
all: lib app app-static
lib: libshared libstatic
libshared: $(LIB_SHARED)
libstatic: $(LIB_STATIC)
app: $(APP)
app-static: $(APP_STATIC)


$(BUILD_DIR)/:
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/
	$(CC) $(CFLAGS) -c $< -o $@

#
#  Shared library
#
$(BUILD_DIR)/lib$(NAME).so: $(BUILD_DIR)/lib$(NAME).so.$(VERSION) $(BUILD_DIR)/lib$(NAME).so.$(MAJOR)
	ln -sf $(notdir $<) $@

$(BUILD_DIR)/lib$(NAME).so.$(MAJOR): $(BUILD_DIR)/lib$(NAME).so.$(VERSION)
	ln -sf $(notdir $<) $@

$(BUILD_DIR)/lib$(NAME).so.$(VERSION): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_SO) $(LDLIBS) $^ -o $@


#
#  Static library
#
$(BUILD_DIR)/lib$(NAME).a: $(OBJS)
	$(AR) cr $@ $^
	ranlib $@


#
#  Application built to use shared library
#
$(APP): $(BUILD_DIR)/lib$(NAME).so $(APP_SRC)
	$(CC) $(CFLAGS) $(APP_SRC) -o $@ -L$(BUILD_DIR) -l$(NAME)


#
#  Standalone application
#
$(APP_STATIC): $(OBJS) $(APP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(APP_OBJ) -o $(APP_STATIC)


#
#  Test binary
#
$(TEST_BIN): $(OBJS) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(TEST_OBJ) -o $(TEST_BIN)


#
#  Utility targets
#
test: $(TEST_BIN)
	$(SAN_ENV) ./$(TEST_BIN)

clean:
	$(RM) $(OBJS) $(APP_OBJ) $(APP) $(APP_STATIC) $(TEST_BIN) $(TEST_OBJ) $(LIB_STATIC) $(LIB_SHARED) $(DEPS)

clean-test:
	$(RM) $(OBJS) $(APP_OBJ) $(APP) $(APP_STATIC) $(TEST_BIN) $(TEST_OBJ) $(LIB_STATIC) $(LIB_SHARED) $(DEPS)


install: install-static install-shared

install-headers:
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 0644 $(NAME).h $(DESTDIR)$(PREFIX)/include

install-static: libstatic install-headers
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0644 $(BUILD_DIR)/lib$(NAME).a $(DESTDIR)$(LIBDIR)

install-shared: libshared install-headers
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0644 $(BUILD_DIR)/lib$(NAME).so.$(VERSION) $(DESTDIR)$(LIBDIR)
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
