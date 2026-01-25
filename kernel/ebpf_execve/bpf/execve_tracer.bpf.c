#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

struct event {
    __u32 pid;
    __u32 ppid;
    char comm[16];
};

// ring buffer to get data out
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

// attach to the tracepoint
SEC("tp/syscalls/sys_enter_execve")
int handle_execve(void *ctx)
{
    struct event *e;
    __u64 pid_tgid;
    __u32 pid, ppid;

    pid_tgid = bpf_get_current_pid_tgid();
    pid = pid_tgid >> 32;
    ppid = pid_tgid; 
    
    // allocate on the ring buffer
    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e)
        return 0; 

    e->pid = pid;
    e->ppid = ppid;
    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    // sends to userspace
    bpf_ringbuf_submit(e, 0);

    return 0;
}
