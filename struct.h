#ifndef STRUCT_H
#define STRUCT_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct ieee80211_radiotap_header {
    uint8_t version;
    uint8_t pad;
    uint16_t len;
    uint32_t present;
} ieee80211_radiotap_header;

// IEEE 802.11
typedef struct ieee80211_header {
    uint16_t frame_control; // 프레임 컨트롤 필드
    uint16_t duration;      // 기간 필드
    uint8_t dest_addr[6];   // 수신 MAC 주소
    uint8_t src_addr[6];    // 송신 MAC 주소
    uint8_t bssid[6];       // BSSID
    uint16_t seq_ctrl;      // 시퀀스 컨트롤
} ieee80211_header;

// Authentication 프레임
typedef struct auth_frame {
    ieee80211_radiotap_header radiotap_header; // 라디오 탭 헤더
    ieee80211_header hdr; // 802.11 MAC 헤더
    uint16_t auth_algorithm; // 인증 알고리즘
    uint16_t auth_seq; // 인증 순서
    uint16_t status_code; // 상태 코드
} auth_frame;

typedef struct disassoc_frame {
    ieee80211_radiotap_header radiotap_header; // 라디오 탭 헤더
    ieee80211_header hdr; // 802.11 MAC 헤더
    uint16_t reason_code; // 이유 코드
} disassoc_frame;

#pragma pack(pop)

#endif /* STRUCT_H */
