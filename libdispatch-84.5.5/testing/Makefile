# No workie: dispatch_sema
TESTS=	dispatch_apply \
	dispatch_api \
	dispatch_c99 \
	dispatch_cffd \
	dispatch_deadname \
	dispatch_debug \
	queue_finalizer \
	dispatch_group \
	dispatch_overcommit \
	dispatch_pingpong \
	dispatch_plusplus \
	dispatch_priority \
	dispatch_priority2 \
	dispatch_proc \
	dispatch_read \
	dispatch_read2 \
	dispatch_after \
	dispatch_timer \
	dispatch_sema \
	dispatch_suspend_timer \
	dispatch_timer_bit31 \
	dispatch_timer_bit63 \
	dispatch_timer_oneshot \
	dispatch_timer_set_time \
	dispatch_starfish \
	dispatch_cascade \
	dispatch_drift \
	dispatch_readsync \
	nsoperation

all: harness summarize bench $(TESTS)
	@lipo -remove x86_64 -output dispatch_timer_bit31 dispatch_timer_bit31 2>/dev/null || true

logs: $(addsuffix .testlog, $(TESTS))
debuglogs: $(addsuffix .debuglog, $(TESTS))

testbots:
	$(MAKE) test

test: clean-logs
	$(MAKE) _test

_test: all logs debuglogs
	@cat *.testlog *.debuglog
	@cat *.testlog *.debuglog | ./summarize

# Override ARCHS and SDKROOT to cross-build test suite

SRCS = dispatch_test.c
OBJS = $(SRCS:%.c=%.o)
ARCHS=i386 x86_64 ppc
CFLAGS = -Werror -Wall -Wextra -Wshadow -mdynamic-no-pic -Os -g $(patsubst %, -arch %,$(ARCHS))
CPPFLAGS = $(CFLAGS)
LDFLAGS = $(patsubst %, -arch %,$(ARCHS))
LDLIBS = -lstdc++

ifneq ($(SDKROOT),)
CFLAGS += -isysroot $(SDKROOT)
LDFLAGS += -isysroot $(SDKROOT)
CC = xcrun -sdk $(SDKROOT) gcc
endif

harness: harness.o $(OBJS)
summarize: summarize.o
bench: bench.o func.o
	$(CC) $(LDFLAGS) -framework Foundation $(LDLIBS) -o $@ $^

bench.o: bench.mm
	$(CC) -x objective-c++ $(CFLAGS) -c $^ -o $@
func.o: func.c
	$(CC) -x c++ $(CFLAGS) -c $^ -o $@

dispatch_apply: dispatch_apply.o $(OBJS)
dispatch_api: dispatch_api.o $(OBJS)
dispatch_c99: dispatch_c99.o $(OBJS)
dispatch_cffd: dispatch_cffd.o $(OBJS)
	$(CC) $(LDFLAGS) -framework CoreFoundation -o $@ $^
dispatch_deadname: dispatch_deadname.o $(OBJS)
dispatch_debug: dispatch_debug.o $(OBJS)
dispatch_group: dispatch_group.o $(OBJS)
dispatch_overcommit: dispatch_overcommit.o $(OBJS)
dispatch_pingpong: dispatch_pingpong.o $(OBJS)
dispatch_plusplus: dispatch_plusplus.o $(OBJS)
dispatch_priority: dispatch_priority.o $(OBJS)
dispatch_priority2: dispatch_priority2.o $(OBJS)
dispatch_proc: dispatch_proc.o $(OBJS)
queue_finalizer: queue_finalizer.o $(OBJS)
dispatch_read:  dispatch_read.o $(OBJS)
dispatch_read2: dispatch_read2.o $(OBJS)
dispatch_after: dispatch_after.o $(OBJS)
dispatch_timer: dispatch_timer.o $(OBJS)
dispatch_suspend_timer: dispatch_suspend_timer.o $(OBJS)
dispatch_sema: dispatch_sema.o $(OBJS)
dispatch_timer_bit31: dispatch_timer_bit31.o $(OBJS)
dispatch_timer_bit63: dispatch_timer_bit63.o $(OBJS)
dispatch_timer_oneshot: dispatch_timer_oneshot.o $(OBJS)
dispatch_timer_set_time: dispatch_timer_set_time.o $(OBJS)
dispatch_drift: dispatch_drift.o $(OBJS)
dispatch_starfish: dispatch_starfish.o $(OBJS)
dispatch_cascade: dispatch_cascade.o $(OBJS)
dispatch_timer_bit31: dispatch_timer_bit31.o $(OBJS)
dispatch_readsync: dispatch_readsync.o $(OBJS)
ENVIRON_nsoperation = NOLEAKS=1
nsoperation: nsoperation.o $(OBJS)
	$(CC) $(LDFLAGS) -framework Foundation -o $@ $^

dispatch_api.o: dispatch_api.c
	$(CC) -c $(CFLAGS) -include $(SDKROOT)/usr/include/dispatch/dispatch.h -pendantic -o $@ $^

dispatch_c99.o: dispatch_c99.c
	$(CC) -c $(CFLAGS) -std=c99 -pedantic -o $@ $^

dispatch_priority2.o: dispatch_priority.c
	$(CC) -c $(CFLAGS) -DUSE_SET_TARGET_QUEUE=1 -o $@ $^

$(addsuffix .testlog, $(TESTS)): harness $(TESTS)
	$(ENVIRON_$(basename $@)) ./harness ./$(basename $@) > $@

$(addsuffix .debuglog, $(TESTS)): harness $(TESTS)
	$(ENVIRON_$(basename $@)) DYLD_IMAGE_SUFFIX=_debug ./harness ./$(basename $@) > $@

clean-logs:
	rm -f *.testlog *.debuglog *.leakslog

clean: clean-logs
	rm -f *.o *.dSYM bench harness summarize $(TESTS)
