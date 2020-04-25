BIN = gs1encoders-linux.bin

CFLAGS = -g -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wconversion -Wformat -Wformat-security -pedantic
LDFLAGS = -Wl,--as-needed -Wl,-z,relro -Wl,-z,now

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(BIN)
