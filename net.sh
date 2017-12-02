sudo socat udp-listen:420 stdout | ./djinn | socat stdin udp-sendto:10.100.23.169:421

# sudo socat udp-listen:420 stdout
# socat stdin udp-sendto:10.100.23.169:421
