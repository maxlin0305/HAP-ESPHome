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

        hap_serv_t* service = nullptr;
        hap_char_t* char_val = nullptr;

        switch (obj->get_class()) {
          case sensor::Sensor::CLASS_TEMPERATURE:
            service = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_TEMPERATURE_SENSOR);
            char_val = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_CURRENT_TEMPERATURE);
            break;
          case sensor::Sensor::CLASS_HUMIDITY:
            service = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_HUMIDITY_SENSOR);
            char_val = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY);
            break;
          case sensor::Sensor::CLASS_AIR_QUALITY:
            service = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_AIR_QUALITY_SENSOR);
            char_val = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_AIR_QUALITY);
            break;
          default:
            return;
        }

        if (char_val) {
          hap_val_t state;
          state.f = value;
          hap_char_update_val(char_val, &state);
        }
      }

    public:
      SensorEntity(sensor::Sensor* sensorPtr) : HAPEntity({{MODEL, "HAP-SENSOR"}}), sensorPtr(sensorPtr) {}

      void setup() {
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

        hap_serv_t* service = nullptr;

        // 建立對應 HomeKit service
        switch (sensorPtr->get_class()) {
          case sensor::Sensor::CLASS_TEMPERATURE:
            service = hap_serv_create(HAP_SERV_UUID_TEMPERATURE_SENSOR);
            break;
          case sensor::Sensor::CLASS_HUMIDITY:
            service = hap_serv_create(HAP_SERV_UUID_HUMIDITY_SENSOR);
            break;
          case sensor::Sensor::CLASS_AIR_QUALITY:
            service = hap_serv_create(HAP_SERV_UUID_AIR_QUALITY_SENSOR);
            break;
          default:
            ESP_LOGW(TAG, "Unsupported sensor class for '%s'", accessory_name.c_str());
            return;
        }

        hap_serv_set_priv(service, sensorPtr);
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
