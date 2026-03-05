ubuntu22@NYX:~/webrtc$ sysctl net.ipv4.tcp_congestion_control
net.ipv4.tcp_congestion_control = cubic

ubuntu20@NYX:/proc/sys/net/ipv4$ ls
cipso_cache_bucket_size            ip_local_reserved_ports           tcp_fastopen                        tcp_plb_suspend_rto_sec
cipso_cache_enable                 ip_no_pmtu_disc                   tcp_fastopen_blackhole_timeout_sec  tcp_probe_interval
cipso_rbm_optfmt                   ip_nonlocal_bind                  tcp_fastopen_key                    tcp_probe_threshold


net/ipv4/sysctl_net_ipv4.c
	hdr = register_net_sysctl(&init_net, "net/ipv4", ipv4_table);
