
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

using namespace std;
using namespace chrono;

const int print_timer_period = 1;
int main(int argc, char const *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <IP address> <Port>" << endl;
        return 1;
    }

    const char* ip_address = argv[1];
    int port = atoi(argv[2]);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // 创建socket文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 绑定socket到指定的IP地址和端口
    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip_address);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听端口
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // 接受连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    double start_tsc = (double)(clock()) / CLOCKS_PER_SEC;
    double prev_tsc = 0.0;
    double start_cap_tsc = 0.0;
    unsigned int rx_pkts = 0;

  
  while(1) {
    double curr_tsc = (double)(clock()) / CLOCKS_PER_SEC;
    double diff_tsc = (curr_tsc - prev_tsc);
    // read packet from buffer
    int ret = read(new_socket, buffer, 1024);
    
    // after 3 sceond, start capture
    if (curr_tsc - start_tsc < 3) continue;
    else if (start_cap_tsc == 0) start_cap_tsc = curr_tsc;
    
    // pkt accumulated
    if (ret > 0) rx_pkts += 1;

    // print stats every 1 second
    if (diff_tsc >= print_timer_period)
    {   
        double spend_time = curr_tsc - start_cap_tsc; 
        double send_pps = rx_pkts  / spend_time;
        printf("recv %.2f Kpps\n",send_pps/1000);
        prev_tsc = curr_tsc;
    }
  }
}