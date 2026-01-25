#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

// same struct as our kernel module
// TODO: put it in a header
struct event {
    __u32 pid;
    __u32 ppid;
    char comm[16];
};

static volatile bool exiting = false;

static void sig_handler(int sig)
{
    exiting = true;
}

// called when data on the ring buffer
static int handle_event(void *ctx, void *data, size_t data_sz)
{
    const struct event *e = data;
    
    printf("[PID %d] Executed: %s\n", e->pid, e->comm);
    
    return 0;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj = NULL;
    struct bpf_program *prog;
    struct bpf_link *link = NULL;
    struct ring_buffer *rb = NULL;
    int err = 0;
    int map_fd;

    // exit clean
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // open our tracer
    obj = bpf_object__open_file("bpf/execve_tracer.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object file\n");
        return 1;
    }

    // load into the kernel, the kernel checks it
    // TODO: how?
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }

    // get our program handle
    prog = bpf_object__find_program_by_name(obj, "handle_execve");
    if (!prog) {
        fprintf(stderr, "Failed to find BPF program\n");
        err = 1;
        goto cleanup;
    }

    // tell the kernel we actually want to trace this
    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "Failed to attach BPF program\n");
        err = 1;
        goto cleanup;
    }

    // use our bpf object to find our ring buffer
    map_fd = bpf_object__find_map_fd_by_name(obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "Failed to find events map\n");
        err = 1;
        goto cleanup;
    }

    // tell it what call back to use
    rb = ring_buffer__new(map_fd, handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        err = 1;
        goto cleanup;
    }

    printf("Tracing execve() syscalls... Press Ctrl+C to exit.\n");

    // poll for results
    while (!exiting) {
        err = ring_buffer__poll(rb, 100);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            fprintf(stderr, "Error polling ring buffer: %d\n", err);
            break;
        }
    }

cleanup:
    ring_buffer__free(rb);
    bpf_link__destroy(link);
    bpf_object__close(obj);
    
    return err < 0 ? -err : 0;
}
