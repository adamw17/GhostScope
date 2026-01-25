# Makefile for GhostScope eBPF execve tracer

KDIR ?= /usr/include/x86_64-linux-gnu
PWD := $(shell pwd)

CC := clang
BPF_DIR := bpf
USER_DIR := userspace

BPF_OBJ := $(BPF_DIR)/execve_tracer.bpf.o
USER_BIN := execve_tracer

BPF_CFLAGS := -target bpf -I$(KDIR) -g -O2 -Wall

USER_CFLAGS := -g3 -Wall
USER_LDFLAGS := -lbpf -lelf -lz

.PHONY: all clean

all: $(BPF_OBJ) $(USER_BIN)

$(BPF_OBJ): $(BPF_DIR)/execve_tracer.bpf.c
	$(CC) $(BPF_CFLAGS) -c $< -o $@

$(USER_BIN): $(USER_DIR)/execve_tracer.c $(BPF_OBJ)
	$(CC) $(USER_CFLAGS) -o $@ $< $(USER_LDFLAGS)

clean:
	rm -f $(BPF_OBJ) $(USER_BIN)
