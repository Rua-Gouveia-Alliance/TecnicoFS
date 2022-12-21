# Makefile
# Sistemas Operativos, DEI/IST/ULisboa
#
# This makefile should be run from the *root* of the project

CC ?= gcc
LD ?= gcc

# space separated list of directories with header files
INCLUDE_DIRS := fs protocol utils producer-consumer .
# this creates a space separated list of -I<dir> where <dir> is each of the values in INCLUDE_DIRS
INCLUDES = $(addprefix -I, $(INCLUDE_DIRS))


SOURCES  := $(wildcard */*.c)
HEADERS  := $(wildcard */*.h)
OBJECTS  := $(SOURCES:.c=.o)

TARGET_EXECS := mbroker/mbroker manager/manager publisher/pub subscriber/sub

TEST_SOURCES  := $(wildcard tests/*.c)
TEST_TARGETS  := $(TEST_SOURCES:.c=)

MBROKER_SOURCES  := $(wildcard mbroker/*.c)
FS_SOURCES  := $(wildcard fs/*.c)
MANAGER_SOURCES  := $(wildcard manager/*.c)
PRODUCER_CONSUMER_SOURCES  := $(wildcard producer-consumer/*.c)
PROTOCOL_SOURCES  := $(wildcard protocol/*.c)
PUBLISHER_SOURCES  := $(wildcard publisher/*.c)
SUBSCRIBER_SOURCES  := $(wildcard subscriber/*.c)
UTILS_SOURCES  := $(wildcard utils/*.c)

MBROKER_OBJECTS := $(MBROKER_SOURCES:.c=.o)
FS_OBJECTS := $(FS_SOURCES:.c=.o)
MANAGER_OBJECTS := $(MANAGER_SOURCES:.c=.o)
PRODUCER_CONSUMER_OBJECTS := $(PRODUCER_CONSUMER_SOURCES:.c=.o)
PROTOCOL_OBJECTS := $(PROTOCOL_SOURCES:.c=.o)
PUBLISHER_OBJECTS := $(PUBLISHER_SOURCES:.c=.o)
SUBSCRIBER_OBJECTS := $(SUBSCRIBER_SOURCES:.c=.o)
UTILS_OBJECTS := $(UTILS_SOURCES:.c=.o)

# VPATH is a variable used by Makefile which finds *sources* and makes them available throughout the codebase
# vpath %.h <DIR> tells make to look for header files in <DIR>
vpath # clears VPATH
vpath %.h $(INCLUDE_DIRS)

CFLAGS = -std=c17 -D_POSIX_C_SOURCE=200809L
CFLAGS += $(INCLUDES)
LDFLAGS = -pthread

# Warnings
CFLAGS += -fdiagnostics-color=always -Wall -Werror -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused
# Warning suppressions
CFLAGS += -Wno-sign-compare

# optional debug symbols: run make DEBUG=no to deactivate them
ifneq ($(strip $(DEBUG)), no)
  CFLAGS += -g
endif

# optional O3 optimization symbols: run make OPTIM=no to deactivate them
ifeq ($(strip $(OPTIM)), no)
  CFLAGS += -O0
else
  CFLAGS += -O3
endif


# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean depend fmt

all: $(TARGET_EXECS)

test: $(TEST_TARGETS)

# The following target can be used to invoke clang-format on all the source and header
# files. clang-format is a tool to format the source code based on the style specified
# in the file '.clang-format'.
# More info available here: https://clang.llvm.org/docs/ClangFormat.html

# The $^ keyword is used in Makefile to refer to the right part of the ":" in the
# enclosing rule. See https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

fmt: $(SOURCES) $(HEADERS)
	clang-format -i $^



# Note the lack of a rule.
# make uses a set of default rules, one of which compiles C binaries
# the CC, LD, CFLAGS and LDFLAGS are used in this rule
mbroker/mbroker: $(FS_OBJECTS) $(MBROKER_OBJECTS) $(PROTOCOL_OBJECTS) $(PRODUCER_CONSUMER_OBJECTS) $(UTILS_OBJECTS)
manager/manager: $(MANAGER_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)
publisher/pub: $(PUBLISHER_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)
subscriber/sub: $(SUBSCRIBER_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)

clean:
	rm -f $(OBJECTS) $(TARGET_EXECS)


# This generates a dependency file, with some default dependencies gathered from the include tree
# The dependencies are gathered in the file autodep. You can find an example illustrating this GCC feature, without Makefile, at this URL: https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/GCC/options/MM
# Run `make depend` whenever you add new includes in your files
depend : $(SOURCES)
	$(CC) $(INCLUDES) -MM $^ > autodep
