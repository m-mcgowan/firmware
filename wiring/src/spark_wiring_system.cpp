
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_logging.h"
#include "system_task.h"
#include "system_control.h"
#include "system_network.h"

#if Wiring_LogConfig
extern bool(*log_process_config_request_callback)(char*, size_t, size_t, size_t*, DataFormat);
#endif

SystemClass System;

void SystemClass::factoryReset(void)
{
    //This method will work only if the Core is supplied
    //with the latest version of Bootloader
    HAL_Core_Factory_Reset();
}

void SystemClass::dfu(bool persist)
{
    // true  - DFU mode persist if firmware upgrade is not completed
    // false - Briefly enter DFU bootloader mode (works with latest bootloader only )
    //         Subsequent reset or power off-on will execute normal firmware
    HAL_Core_Enter_Bootloader(persist);
}

void SystemClass::reset(void)
{
    reset(0);
}

void SystemClass::reset(uint32_t data)
{
    HAL_Core_System_Reset_Ex(RESET_REASON_USER, data, nullptr);
}

SleepResult SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds, SleepNetworkFlag network)
{
    system_sleep(sleepMode, seconds, network.flag(), NULL);
    System.sleepResult_ = SleepResult();
    return System.sleepResult_;
}

SleepResult SystemClass::sleepPinImpl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, SleepNetworkFlag flags) {
    int ret = system_sleep_pins(pins, pins_count, modes, modes_count, seconds, flags.flag(), nullptr);
    System.sleepResult_ = SleepResult(ret, pins, pins_count);
    return System.sleepResult_;
}

uint32_t SystemClass::freeMemory()
{
    runtime_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_Core_Runtime_Info(&info, NULL);
    return info.freeheap;
}

#if Wiring_LogConfig
bool SystemClass::enableFeature(LoggingFeature) {
    log_process_config_request_callback = spark::logProcessConfigRequest;
    return true;
}
#endif

SleepResult::SleepResult(int ret, const pin_t* pins, size_t pinsSize) {
    if (ret > 0) {
        // pin
        --ret;
        if ((size_t)ret < pinsSize) {
            pin_ = pins[ret];
            reason_ = WAKEUP_REASON_PIN;
            err_ = SYSTEM_ERROR_NONE;
        }
    } else if (ret == 0) {
        reason_ = WAKEUP_REASON_RTC;
        err_ = SYSTEM_ERROR_NONE;
    } else {
        err_ = static_cast<system_error_t>(ret);
    }
}

WakeupReason SleepResult::reason() const {
    return reason_;
}

bool SleepResult::wokenUpByRtc() const {
    return reason_ == WAKEUP_REASON_RTC || reason_ == WAKEUP_REASON_PIN_OR_RTC;
}

bool SleepResult::wokenUpByPin() const {
    return reason_ == WAKEUP_REASON_PIN || reason_ == WAKEUP_REASON_PIN_OR_RTC;
}

pin_t SleepResult::pin() const {
    return pin_;
}

bool SleepResult::rtc() const {
    return wokenUpByRtc();
}

system_error_t SleepResult::error() const {
    return err_;
}
