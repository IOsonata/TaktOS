ROOT := ..
INCLUDES := -I$(ROOT)/include -I$(ROOT)/ARM/include
CC ?= clang
CXX ?= clang++

C_EXAMPLES := \
	basic/basic_c.c \
	queue/producer_consumer_c.c \
	mutex/mutex_pi_c.c \
	posix/posix_pse51.c \
	mpu_guard.c

CPP_EXAMPLES := \
	basic/basic_cpp.cpp \
	queue/producer_consumer.cpp \
	mutex/mutex_pi.cpp \
	posix/posix_pse51_cpp.cpp

.PHONY: check clean

check:
	@for f in $(C_EXAMPLES); do \
		echo "checking $$f"; \
		$(CC) -std=gnu11 -fsyntax-only $(INCLUDES) $$f; \
	done
	@for f in $(CPP_EXAMPLES); do \
		echo "checking $$f"; \
		$(CXX) -std=gnu++17 -fsyntax-only $(INCLUDES) $$f; \
	done
	@echo "all examples passed syntax verification"

clean:
	@true
