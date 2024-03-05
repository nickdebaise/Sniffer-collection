//
// Created by Nicholas Debaise on 2/15/24.
//

#ifndef SNIFFER_MAIN_H
#define SNIFFER_MAIN_H
#include <unordered_set>
#include <ArduinoJson.h>
/* ======= */
/* DEFINES */
/* ======= */
#define BLINK_INTERVAL 1000  // Blink duration for wifi scanning LED
#define RSSI_MIN (-120)
#define RSSI_MAX (-40)
#define RSSI_BINS 40  // Number of bins

#define BUFFER_SIZE 9

/* ============ */
/* Type definitions */
/* ============ */
typedef struct {
    unsigned frame_ctrl : 16;
    unsigned duration_id : 16;
    uint8_t addr1[6];  // receiver address
    uint8_t addr2[6];  // sender address
    uint8_t addr3[6];  // filtering address
    unsigned sequence_ctrl : 16;
    uint8_t addr4[6];  // optional
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];  // network data ended with 4 bytes csum (CRC32)
} wifi_ieee80211_packet_t;


/* ============= */
/* Global variables */
/* ============= */

extern int bufferIndex;

extern std::array<std::unordered_set<std::string>, BUFFER_SIZE> uniqueDevicesBuffer;
extern std::array<std::unordered_set<std::string>, BUFFER_SIZE> randomDevicesBuffer;

extern short N_r[BUFFER_SIZE][RSSI_BINS];
extern short N_v[BUFFER_SIZE][RSSI_BINS];

#endif //SNIFFER_MAIN_H
