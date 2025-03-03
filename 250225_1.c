#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <time.h>

#define MULTICAST_GROUP "224.0.0.1"
#define PORT 6000
#define RESPONSE_PORT 6001
#define CONTROL_PORT 6002
#define MAX_BUF_SIZE 256
#define RANDOM_NUM_LENGTH 16

void get_eth0_ip(char *ip_buffer, size_t size) {
    struct ifaddrs *ifaddr, *ifa;
    int found = 0;
    getifaddrs(&ifaddr);
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "eth0") == 0) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip_buffer, size);
            found = 1;
            break;
        }
    }
    freeifaddrs(ifaddr);

    if (!found) {
        snprintf(ip_buffer, size, "Unknown");
    }
}


void execute_control_command(const char *new_ip, const char *subnet_mask, const char *gateway) {
    printf("Executing IP change command: New IP: %s, Subnet Mask: %s, Gateway: %s\n", new_ip, subnet_mask, gateway);

    char command[512];
    snprintf(command, sizeof(command), "sudo ifconfig eth0 %s netmask %s", new_ip, subnet_mask);
    
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to execute IP change command\n");
        return;
    }
    printf("Successfully changed IP to %s with subnet mask %s\n", new_ip, subnet_mask);

    snprintf(command, sizeof(command), "sudo ip route add default via %s dev eth0", gateway);
    ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to set gateway\n");
    } else {
        printf("Successfully set gateway to %s\n", gateway);
    }

    system("touch 1");
}

void generate_random_id(char *random_id, size_t size) {
    FILE *urandom = fopen("/dev/urandom", "r");
    if (!urandom) {
        perror("Failed to open /dev/urandom");
        exit(1);
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long long timestamp = (unsigned long long)ts.tv_sec * 1000000000 + ts.tv_nsec;

    unsigned char random_bytes[8];
    fread(random_bytes, 1, sizeof(random_bytes), urandom);

    fclose(urandom);

    snprintf(random_id, size, "%016llX%02X%02X%02X%02X", timestamp, random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3]);
}

int main() {
    int sockfd, control_sock;
    struct sockaddr_in addr, sender_addr;
    char buffer[MAX_BUF_SIZE];
    char random_id[RANDOM_NUM_LENGTH + 1];

    generate_random_id(random_id, sizeof(random_id));
    printf("Device Random ID: %s\n", random_id);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    control_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (control_sock < 0) {
        perror("control socket");
        close(sockfd);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        close(control_sock);
        return 1;
    }

    struct sockaddr_in control_addr;
    memset(&control_addr, 0, sizeof(control_addr));
    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if (bind(control_sock, (struct sockaddr*)&control_addr, sizeof(control_addr)) < 0) {
        perror("control bind");
        close(sockfd);
        close(control_sock);
        return 1;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        close(sockfd);
        close(control_sock);
        return 1;
    }

    printf("Waiting for multicast messages...\n");

    fd_set read_fds;
    int max_fd = (sockfd > control_sock) ? sockfd : control_sock;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(control_sock, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        socklen_t sender_len = sizeof(sender_addr);

        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUF_SIZE, 0,
                                        (struct sockaddr*)&sender_addr, &sender_len);
            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                printf("Received multicast: %s\n", buffer);

                if (strcmp(buffer, "from -> Test_SADP_Qt_1") == 0) {
                    char ip_addr[INET_ADDRSTRLEN] = "Unknown";
                    get_eth0_ip(ip_addr, sizeof(ip_addr));

                    int response_sock = socket(AF_INET, SOCK_DGRAM, 0);
                    struct sockaddr_in response_addr;
                    response_addr.sin_family = AF_INET;
                    response_addr.sin_port = htons(RESPONSE_PORT);
                    response_addr.sin_addr = sender_addr.sin_addr;

                    snprintf(buffer, sizeof(buffer), "%s,%s", ip_addr, random_id);
                    sendto(response_sock, buffer, strlen(buffer), 0, 
                           (struct sockaddr*)&response_addr, sizeof(response_addr));
                    close(response_sock);
                    printf("Sent eth0 IP: %s and Random ID: %s\n", ip_addr, random_id);
                }
            }
        }

        if (FD_ISSET(control_sock, &read_fds)) {
            ssize_t recv_len = recvfrom(control_sock, buffer, MAX_BUF_SIZE, 0,
                                        (struct sockaddr*)&sender_addr, &sender_len);
            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                printf("Received control command: %s\n", buffer);

                char *command = strtok(buffer, ",");
                char *receivedRandomId = strtok(NULL, ",");
                char *new_ip = strtok(NULL, ",");
                char *subnet_mask = strtok(NULL, ",");
                char *gateway = strtok(NULL, ","); 

                if (receivedRandomId && strcmp(receivedRandomId, random_id) == 0) {
                    if (strcmp(command, "SET_IP") == 0 && new_ip && subnet_mask && gateway) {
                        execute_control_command(new_ip, subnet_mask, gateway);
                    }
                } else {
                    printf("Received SET_IP command, but ID mismatch\n");
                }
            }
        }
    }

    close(sockfd);
    close(control_sock);
    return 0;
}
