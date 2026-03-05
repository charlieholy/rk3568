ubuntu22@NYX:~/webrtc$ sysctl net.ipv4.tcp_congestion_control
net.ipv4.tcp_congestion_control = cubic

ubuntu20@NYX:/proc/sys/net/ipv4$ ls
cipso_cache_bucket_size            ip_local_reserved_ports           tcp_fastopen                        tcp_plb_suspend_rto_sec
cipso_cache_enable                 ip_no_pmtu_disc                   tcp_fastopen_blackhole_timeout_sec  tcp_probe_interval
cipso_rbm_optfmt                   ip_nonlocal_bind                  tcp_fastopen_key                    tcp_probe_threshold


net/ipv4/sysctl_net_ipv4.c
	hdr = register_net_sysctl(&init_net, "net/ipv4", ipv4_table);


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHECK_RET(ret, msg) \
    if (ret < 0) { perror(msg); exit(1); }

void get_cc(int fd, const char *name) {
    char cc[128] = {0};
    socklen_t len = sizeof(cc);
    int ret = getsockopt(fd, IPPROTO_TCP, TCP_CONGESTION, cc, &len);
    CHECK_RET(ret, "getsockopt");
    printf("[%s] 拥塞算法：%s\n", name, cc);
}

void set_cc(int fd, const char *cc_name, const char *name) {
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_CONGESTION, cc_name, strlen(cc_name));
    CHECK_RET(ret, "setsockopt");
    printf("[%s] 设置成功：%s\n", name, cc_name);
}

int main() {
    int fd1, fd2;

    // Socket 1：reno
    fd1 = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(fd1, "socket1");
    set_cc(fd1, "reno", "Socket-1");
    get_cc(fd1, "Socket-1");

    // Socket 2：cubic
    fd2 = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_RET(fd2, "socket2");
    set_cc(fd2, "cubic", "Socket-2");
    get_cc(fd2, "Socket-2");

    // 查看全局默认
    printf("\n===== 全局默认拥塞算法 =====\n");
    system("cat /proc/sys/net/ipv4/tcp_congestion_control");

    close(fd1);
    close(fd2);
    return 0;
}
//
ubuntu20@NYX:~/test$ ./a.out
[Socket-1] 设置成功：reno
[Socket-1] 拥塞算法：reno
[Socket-2] 设置成功：cubic
[Socket-2] 拥塞算法：cubic

===== 全局默认拥塞算法 =====
reno
