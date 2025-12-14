#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"

#include "hx711.h"

static const char* DTAG = "discord_bot";
static int32_t g_discord_event;
static discord_event_data_t* g_discord_event_data;
static discord_handle_t g_bot;

static void bot_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    static const char* TAG = "discord_bot";
    g_discord_event = event_id
    g_discord_event_data = (discord_event_data_t*) event_data;

    /*
    discord_event_data_t* data = (discord_event_data_t*) event_data;
    switch(event_id) {
        case DISCORD_EVENT_CONNECTED: {
                discord_session_t* session = (discord_session_t*) data->ptr;

                ESP_LOGI(TAG, "Bot %s#%s connected",
                    session->user->username,
                    session->user->discriminator
                );
            }
            break;
        
        case DISCORD_EVENT_MESSAGE_RECEIVED: {
                discord_message_t* msg = (discord_message_t*) data->ptr;

                ESP_LOGI(TAG, "New message (dm=%s, autor=%s#%s, bot=%s, channel=%s, guild=%s, content=%s)",
                    ! msg->guild_id ? "true" : "false",
                    msg->author->username,
                    msg->author->discriminator,
                    msg->author->bot ? "true" : "false",
                    msg->channel_id,
                    msg->guild_id ? msg->guild_id : "NULL",
                    msg->content
                );

                char* echo_content = estr_cat("Hey ", msg->author->username, " you wrote `", msg->content, "`");

                discord_message_t echo = {
                    .content = echo_content,
                    .channel_id = msg->channel_id
                };

                discord_message_t* sent_msg = NULL;
                esp_err_t err = discord_message_send(g_bot, &echo, &sent_msg);
                free(echo_content);

                if(err == ESP_OK) {
                    ESP_LOGI(TAG, "Echo message successfully sent");

                    if(sent_msg) { // null check because message can be sent but not returned
                        ESP_LOGI(TAG, "Echo message got ID #%s", sent_msg->id);
                        discord_message_free(sent_msg);
                    }
                } else {
                    ESP_LOGE(TAG, "Fail to send echo message");
                }
            }
            break;
        
        case DISCORD_EVENT_MESSAGE_UPDATED: {
                discord_message_t* msg = (discord_message_t*) data->ptr;
                ESP_LOGI(TAG, "%s has updated his message (#%s). New content: %s", msg->author->username, msg->id, msg->content);
            }
            break;
        
        case DISCORD_EVENT_MESSAGE_DELETED: {
                discord_message_t* msg = (discord_message_t*) data->ptr;
                ESP_LOGI(TAG, "Message #%s deleted", msg->id);
            }
            break;
        
        case DISCORD_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Bot logged out");
            break;
    }
    */
}

void hx711_event_handler(void *pvParameters) {
    static const char* TAG = "hx711";
    const hx711_t dev = {
        .dout = CONFIG_EXAMPLE_DOUT_GPIO,
        .pd_sck = CONFIG_EXAMPLE_PD_SCK_GPIO,
        .gain = HX711_GAIN_A_64
    };

    // initialize device
    ESP_ERROR_CHECK(hx711_init(&dev));

    // read from device
    while (1) {
        esp_err_t r = hx711_wait(&dev, 500);
        if (r != ESP_OK) {
            ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }
        int32_t load_data;
        r = hx711_read_average(&dev, CONFIG_EXAMPLE_AVG_TIMES, &load_data);
        if (r != ESP_OK) {
            ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }
        // debug data values using idf.py monitor
        ESP_LOGI(TAG, "Raw data: %" PRIi32, data);

        discord_message_t* msg = (discord_message_t*) g_discord_event_data->ptr;
        char* echo_content = estr_cat("Raw load cell data: ", load_data);
        discord_message_t echo = {
            .content = echo_content,
            .channel_id = msg->channel_id
        };
        discord_message_t* sent_msg = NULL;
        esp_err_t err = discord_message_send(g_bot, &echo, &sent_msg);
        free(echo_content);

        if(err == ESP_OK) {
            ESP_LOGI(TAG, "Echo message successfully sent");

            if(sent_msg) { // null check because message can be sent but not returned
                ESP_LOGI(TAG, "Echo message got ID #%s", sent_msg->id);
                discord_message_free(sent_msg);
            }
        } else {
            ESP_LOGE(TAG, "Failed to send echo message");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    discord_config_t cfg = {
        .intents = DISCORD_INTENT_GUILD_MESSAGES | DISCORD_INTENT_MESSAGE_CONTENT
    };
    g_bot = discord_create(&cfg);
    ESP_ERROR_CHECK(discord_register_events(g_bot, DISCORD_EVENT_ANY, bot_event_handler, NULL));
    ESP_ERROR_CHECK(discord_login(g_bot));
    xTaskCreate(hx711_event_handler, "hx711_event_handler", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
