#pragma once
#include <esphome/core/defines.h>
#ifdef USE_SENSOR
#include <esphome/core/application.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include "hap_entity.h"

namespace esphome
{
  namespace homekit
  {
    class SensorEntity : public HAPEntity
    {
    private:
      static constexpr const char* TAG = "SensorEntity";
      sensor::Sensor* sensorPtr;

      static int acc_identify(hap_acc_t* ha) {
        ESP_LOGI(TAG, "Accessory identified");
        return HAP_SUCCESS;
      }

      static void on_sensor_update(sensor::Sensor* obj, float value) {
        hap_acc_t* acc = hap_acc_get_by_aid(hap_get_unique_aid(std::to_string(obj->get_object_id_hash()).c_str()));
        if (!acc) return;

        hap_serv_t* service = hap_acc_get_first_serv(acc);
        if (!service) return;

        hap_char_t* char_val = hap_serv_get_first_char(service);
        if (!char_val) return;

        hap_val_t state;
        state.f = value;  // 使用 float 更新
        hap_char_update_val(char_val, &state);

        ESP_LOGD(TAG, "%s updated to %.2f", obj->get_name().c_str(), value);
      }

      static int sensor_read(hap_char_t* hc, hap_status_t* status_code, void* serv_priv, void* read_priv) {
        if (!serv_priv) return HAP_FAIL;

        sensor::Sensor* sensorPtr = (sensor::Sensor*)serv_priv;
        float v = sensorPtr->get_state();
        hap_val_t state;
        state.f = v;
        hap_char_update_val(hc, &state);
        return HAP_SUCCESS;
      }

    public:
      SensorEntity(sensor::Sensor* sensorPtr) : HAPEntity({{MODEL, "HAP-SENSOR"}}), sensorPtr(sensorPtr) {}

      void setup() {
        hap_serv_t* service = nullptr;
        std::string device_class = sensorPtr->get_device_class();

        if (device_class == "temperature") {
          service = hap_serv_temperature_sensor_create(sensorPtr->state);
        } else if (device_class == "humidity") {
          service = hap_serv_humidity_sensor_create(sensorPtr->state);
        } else if (device_class == "illuminance") {
          service = hap_serv_light_sensor_create(sensorPtr->state);
        } else if (device_class == "aqi") {
          service = hap_serv_air_quality_sensor_create(sensorPtr->state);
        } else {
          ESP_LOGW(TAG, "Unsupported sensor class '%s'", device_class.c_str());
          return;
        }

        hap_acc_cfg_t acc_cfg = {
          .name = nullptr,
          .serial_num = nullptr,
          .model = strdup(accessory_info[MODEL]),
          .manufacturer = strdup(accessory_info[MANUFACTURER]),
          .fw_rev = strdup(accessory_info[FW_REV]),
          .hw_rev = strdup("ESP32C3"),
          .pv = strdup("1.1.0"),
          .cid = HAP_CID_SENSOR,
          .identify_routine = acc_identify,
          .pvt = nullptr
        };

        std::string accessory_name = sensorPtr->get_name();
        acc_cfg.name = accessory_info[NAME] ? strdup(accessory_info[NAME])
                                            : strdup(accessory_name.c_str());
        acc_cfg.serial_num = accessory_info[SN] ? strdup(accessory_info[SN])
                                                : strdup(std::to_string(sensorPtr->get_object_id_hash()).c_str());

        hap_acc_t* accessory = hap_acc_create(&acc_cfg);
        hap_serv_set_priv(service, sensorPtr);
        hap_serv_set_read_cb(service, sensor_read);
        hap_acc_add_serv(accessory, service);
        hap_add_bridged_accessory(accessory, hap_get_unique_aid(std::to_string(sensorPtr->get_object_id_hash()).c_str()));

        if (!sensorPtr->is_internal())
          sensorPtr->add_on_value_callback([this](float v) { SensorEntity::on_sensor_update(sensorPtr, v); });

        ESP_LOGI(TAG, "Sensor '%s' linked to HomeKit", accessory_name.c_str());
      }
    };
  }
}
#endif
