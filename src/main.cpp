#include "Arduino.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include <unordered_set>
#include <array>
#include <string>
#include "main.h"
#include "wifi_sniffing.h"


/* ====== */
/* Variable declarations */
/* ====== */

const long wifiScanTime = 9 * 60 * 1000;        // 9 minutes in milliseconds


unsigned long startMillis = millis();
unsigned long currentMillis = startMillis;
unsigned long lastChannelSwitchTime = startMillis;

bool ledState = false;
unsigned long lastBlinkTime = 0;

const int channelSwitchInterval = wifiScanTime / 3;
const int channels[3] = {1, 6, 11};
int currentChannelIndex = 0;

int bufferIndex = 0; // Current index in the circular buffers

std::array<std::unordered_set<std::string>, BUFFER_SIZE> uniqueDevicesBuffer;
std::array<std::unordered_set<std::string>, BUFFER_SIZE> randomDevicesBuffer;

short N_r[BUFFER_SIZE][RSSI_BINS] = {{}};
short N_v[BUFFER_SIZE][RSSI_BINS] = {{}};
short N[BUFFER_SIZE] = {0};


/* ======= */
/* Function Implementations */
/* ======= */
void moveToNextBufferSlot();
void aggregateRSSIArrays(int N_r_aggregated[RSSI_BINS], int N_v_aggregated[RSSI_BINS]);
void aggregateMacAddresses(std::unordered_set<std::string>& aggregatedUniqueDevices, std::unordered_set<std::string>& aggregatedRandomDevices);

void setup() {
    Serial.begin(9600);
    delay(5000);

    Serial.println("------BEGIN------");
    pinMode(LED_BUILTIN, OUTPUT);


    // Setup for WiFi scanning
    Serial.println("Setting up WiFi Station");

    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(WiFiSnifferPacketHandler);
    esp_wifi_set_channel(channels[currentChannelIndex], WIFI_SECOND_CHAN_NONE); // Set initial channel

    WiFi.disconnect();

    Serial.println("WiFi station finished setup");
}

void loop() {
    currentMillis = millis();

    // Switch channel every channelSwitchInterval
    if (currentMillis - lastChannelSwitchTime >= channelSwitchInterval) {
        currentChannelIndex = (currentChannelIndex + 1) % 3; // Cycle through channels 1, 6, 11
        WiFi.disconnect(); // ensure wifi stops operations on curr channel
        esp_wifi_set_channel(channels[currentChannelIndex], WIFI_SECOND_CHAN_NONE);
        Serial.print("Switched to channel ");
        Serial.println(channels[currentChannelIndex]);
        lastChannelSwitchTime = currentMillis;
    }

    if (currentMillis - lastBlinkTime >= BLINK_INTERVAL) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
        lastBlinkTime = currentMillis;
    }

    // move buffer every 60 seconds
    if (currentMillis - startMillis >= 60000) {
        moveToNextBufferSlot();
        Serial.println("Moving to next buffer slot");
        startMillis = currentMillis;

        //aggregate data
        std::unordered_set<std::string> aggregatedUniqueDevices;
        std::unordered_set<std::string> aggregatedRandomDevices;
        aggregateMacAddresses(aggregatedUniqueDevices, aggregatedRandomDevices);

        // aggregate RSSI bins
        int N_r_aggregated[RSSI_BINS] = {0};
        int N_v_aggregated[RSSI_BINS] = {0};
        aggregateRSSIArrays(N_r_aggregated, N_v_aggregated);

        Serial.print("Unique WiFi devices found: ");
        Serial.println(aggregatedUniqueDevices.size());
        Serial.print("Random WiFi MAC addresses found: ");
        Serial.println(aggregatedRandomDevices.size());

        Serial.print("CURL https://us-central1-scholarsproject-1f3ff.cloudfunctions.net/uploadWiFiData -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d ");

        // build request string
        String requestString = "\"numValid=" + String(aggregatedUniqueDevices.size()) + "&numUnique=" + String(aggregatedUniqueDevices.size()) + "&random=" + String(aggregatedRandomDevices.size()) + "&groundTruth=0";
        for (int i = 0; i < RSSI_BINS; ++i) {
            requestString += "&N_v_" + String(i) + "=" + String(N_v_aggregated[i]) + "&N_r_" + String(i) + "=" + String(N_r_aggregated[i]);
        }

        requestString += "\"";

        Serial.println(requestString);
    }

    delay(1000);

}

/**
 * Move to the next buffer slot in the circular buffer which keeps track of the last 9 minutes of data
 */
void moveToNextBufferSlot() {
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
    Serial.println("Next buffer slot: " + String(bufferIndex));

    uniqueDevicesBuffer[bufferIndex].clear();
    randomDevicesBuffer[bufferIndex].clear();

    for (int i = 0; i < RSSI_BINS; ++i) {
        N_r[bufferIndex][i] = 0;
        N_v[bufferIndex][i] = 0;
    }
}

/**
 * Aggregate the MAC addresses from the circular buffer
 * @param aggregatedUniqueDevices
 * @param aggregatedRandomDevices
 */
void aggregateMacAddresses(std::unordered_set<std::string>& aggregatedUniqueDevices, std::unordered_set<std::string>& aggregatedRandomDevices) {
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        aggregatedUniqueDevices.insert(uniqueDevicesBuffer[i].begin(), uniqueDevicesBuffer[i].end());
        aggregatedRandomDevices.insert(randomDevicesBuffer[i].begin(), randomDevicesBuffer[i].end());
    }
}

/**
 * Aggregate the RSSI arrays from the circular buffer
 * @param N_r_aggregated
 * @param N_v_aggregated
 */
void aggregateRSSIArrays(int N_r_aggregated[RSSI_BINS], int N_v_aggregated[RSSI_BINS]) {
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        for (int j = 0; j < RSSI_BINS; ++j) {
            N_r_aggregated[j] += N_r[i][j];
            N_v_aggregated[j] += N_v[i][j];
        }
    }
}