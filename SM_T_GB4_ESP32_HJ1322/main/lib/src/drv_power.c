


#include "esp32/ulp.h"
#include "esp_sleep.h"
#include <time.h>
#include <sys/time.h>
#include "ulp_common.h"

#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp32/ulp.h"
#include "ulp_main.h"

#include "drv_gpio.h"




extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");



/*****************************************************/

static RTC_DATA_ATTR struct timeval sleep_enter_time;



void rt_sleep_wakeup_cause(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause != ESP_SLEEP_WAKEUP_ULP)
    {
        CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN);
    }

    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_EXT1:
    {
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask != 0)
        {
            int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
            printf("-- Wake up from GPIO %d\r\n", pin);
        }
        else
        {
            printf("-- Wake up from GPIO\r\n");
        }
        break;
    }

    case ESP_SLEEP_WAKEUP_TIMER:
    {
        printf("-- Wake up from timer. Time spent in deep sleep: %dms\r\n", sleep_time_ms);
        break;
    }

    case ESP_SLEEP_WAKEUP_ULP:
    {
        printf("-- Deep sleep wakeup\r\n");
        printf( "-- ULP did %d measurements since last reset\r\n", ulp_sample_counter & UINT16_MAX);
        printf("-- Thresholds:  low=%d  high=%d\r\n", ulp_low_thr, ulp_high_thr);
        ulp_last_result &= UINT16_MAX;
        printf("-- Value=%d was %s threshold\r\n", ulp_last_result,
                   ulp_last_result < ulp_low_thr ? "below" : "above");
        break;
    }

    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
        printf("-- Not a deep sleep reset\r\n");
    }
}


/*****************************
**  初始化协处理器程序
*******************************/

static void init_ulp_program(void)
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
            (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /* Configure ADC channel */
    /* Note: when changing channel here, also change 'adc_channel' constant
       in adc.S */
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
#if CONFIG_IDF_TARGET_ESP32
    adc1_config_width(ADC_WIDTH_BIT_12);
#elif CONFIG_IDF_TARGET_ESP32S2
    //adc1_config_width(ADC_WIDTH_BIT_13);
#endif
    adc1_ulp_enable();

    /* Set low and high thresholds, approx. 1.35V - 1.75V*/
    ulp_low_thr = 0;
    ulp_high_thr = 800;

    /* Set ULP wake up period to 20ms */
    ulp_set_wakeup_period(0, 20000);

    /* Disconnect GPIO12 and GPIO15 to remove current drain through
     * pullup/pulldown resistors.
     * GPIO12 may be pulled high to select flash voltage.
     */
    //rtc_gpio_isolate(GPIO_NUM_12);
    //rtc_gpio_isolate(GPIO_NUM_15);
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}




/*******************************
**  进入睡眠模式
********************************/

static void start_ulp_program(void)
{
    /* Reset sample counter */
    ulp_sample_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}




/*******************************
**  进入睡眠模式
********************************/

void enter_deep_sleep_mode(uint32_t sleep_time_sec)
{
    uint64_t wakeup_time_sec = sleep_time_sec;

    printf("-- Enabling timer wakeup, %llds\r\n", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);
    gettimeofday(&sleep_enter_time, NULL);

    // const int ext_wakeup_pin_1 = GPIO_CAN_RX;
    // const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
    // esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);

    // const int ext_wakeup_pin_2 = GPIO_SPI_IRQ;
    // const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;   
    // esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);

    init_ulp_program();
    start_ulp_program();

    if (ESP_OK != esp_sleep_enable_ulp_wakeup())
    {
        printf("-- ERR: func:%s, line(%d)\r\n", __FUNCTION__, __LINE__);
        return;
    }

    esp_deep_sleep_start();
}






