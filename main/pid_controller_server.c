/*
 *  PID controller server
 */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "../../my_wifi.h"  // hide personal data from repository
// #include "commandmanager.h"


/*
 *  From my_wifi.h
 */
#define EXAMPLE_WIFI_SSID MY_SSID
#define EXAMPLE_WIFI_PASS MY_PSWD

#define UDP_PORT 1200


#define REQUEST_RESPONSE_BUF_SIZE (sizeof(char)+2*(sizeof(float)))  // same size for both requests and responses
#define SERVER_TASK_SLEEP_TIME_MS 5
#define NO_MSG_TIMEOUT_SECONDS 15.0



/*
 *  FreeRTOS event group to signal when we are connected & ready to make a request
 */
static EventGroupHandle_t wifi_event_group;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

static const char *TAG = "pid-controller-server";

static esp_err_t event_handler(void *ctx, system_event_t *event) {

    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
        xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");
        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
        ESP_LOGI(TAG, "IPv6: %s", ip6);
    default:
        break;
    }

    return ESP_OK;
}



static void udp_server_task(void *pvParameters) {
    
    char buf[REQUEST_RESPONSE_BUF_SIZE];
    char addr_str[128];
    int addr_family;
    int ip_protocol;


    while (1) {

        #ifdef CONFIG_IPV4
            struct sockaddr_in destAddr;
            destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(UDP_PORT);
            addr_family = AF_INET;
            ip_protocol = IPPROTO_IP;
            inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
        #else  // IPV6
            struct sockaddr_in6 destAddr;
            bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
            destAddr.sin6_family = AF_INET6;
            destAddr.sin6_port = htons(UDP_PORT);
            addr_family = AF_INET6;
            ip_protocol = IPPROTO_IPV6;
            inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        #endif

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket binded");


        while (1) {

            /* Check for available data in socket. Make sure you have CONFIG_LWIP_SO_RCVBUF option set to 'y'
            in your sdkconfig */
            int data_len = 0;
            err = ioctl(sock, FIONREAD, &data_len);
            if (err < 0) {
                ESP_LOGE(TAG, "ioctl: errno %d", errno);
            }
            if (data_len > 0) {

                /*
                *  recvfrom: receive a UDP datagram from a client
                *  len: message byte size
                */
                ESP_LOGI(TAG, "New data");
                struct sockaddr_in6 sourceAddr;  // large enough for both IPv4 or IPv6
                socklen_t socklen = sizeof(sourceAddr);
                int len = recvfrom(sock, buf, REQUEST_RESPONSE_BUF_SIZE, 0, (struct sockaddr *)&sourceAddr, &socklen);

                // error occured during receiving
                if (len < 0) {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                    break;
                }
                // data received
                else {
                    // get the sender's ip address as string
                    if (sourceAddr.sin6_family == PF_INET) {
                        inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    }
                    else if (sourceAddr.sin6_family == PF_INET6) {
                        inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                    }

                    // buf[len] = 0;  // null-terminate whatever we received and treat like a string...
                    ESP_LOGI(TAG, "Received %d bytes from %s", len, addr_str);
                    // ESP_LOGI(TAG, "%s", buf);

                    // process_request(buf);

                    int err = sendto(sock, buf, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                        break;
                    }
                }
            }
            else {
                vTaskDelay(15/portTICK_PERIOD_MS);
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }


    vTaskDelete(NULL);
}



void app_main() {

    ESP_ERROR_CHECK( nvs_flash_init() );


    /*
     *  Initialize Wi-Fi
     */
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );


    /*
     *  Wait for IP
     */
    uint32_t bits = IPV4_GOTIP_BIT | IPV6_GOTIP_BIT ;
    ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");


    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}
