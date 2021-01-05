ifneq ($(filter Darwin,$(shell uname -s)),)
  CFLAGS += -DmacOS
endif

all:	make_logger

clean:
	rm -f make_logger.o make_logger
