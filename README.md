# Лампа/LED контроллер на основе ESP-NOW для ESP8266/ESP8285
Лампа/LED контроллер на основе ESP-NOW для ESP8266/ESP8285. Альтернативная прошивка для Tuya/SmartLife WiFi ламп/LED контроллеров.

## Функции:

1. Никакого WiFi и сторонних серверов. Всё работает исключительно локально.
2. Прошивка может использоваться на многих лампах/LED контроллерах Tuya/SmartLife.
3. Сохранение в памяти последнего состояния при выключении питания. Переход в последнее состояние при включении питания.
4. При подключении к шлюзу периодическая передача своего состояния доступности (Keep Alive) и статуса (ON/OFF). 
5. Управление осуществляется через [ESP-NOW шлюз](https://github.com/aZholtikov/ESP-NOW-MQTT_Gateway) посредством Home Assistant через MQTT брокер.
  
## Примечание:

1. Работает на основе библиотеки [ZHNetwork](https://github.com/aZholtikov/ZHNetwork) и протокола передачи данных [ZH Smart Home Protocol](https://github.com/aZholtikov/ZH-Smart-Home-Protocol).
2. Для работы в сети необходимо наличие [ESP-NOW - MQTT Gateway](https://github.com/aZholtikov/ESP-NOW-MQTT_Gateway).
3. Для включения режима обновления прошивки необходимо послать команду "update" в корневой топик устройства (пример - "homeassistant/espnow_led/70039F44BEF7"). Устройство перейдет в режим обновления (подробности в [API](https://github.com/aZholtikov/ZHNetwork/blob/master/src/ZHNetwork.h) библиотеки [ZHNetwork](https://github.com/aZholtikov/ZHNetwork)). Аналогично для перезагрузки послать команду "restart".
4. При возникновении вопросов/пожеланий/замечаний пишите на github@zh.com.ru

## Внимание!

Для использования этой прошивки на Tuya/SmartLife выключателях WiFi модуль должен быть заменен на ESP8266 совместимый (при необходимости).

## Пример полной конфигурации для Home Assistant:

    light:
    - platform: mqtt
      name: "NAME"
      state_topic: "homeassistant/espnow_led/70039F44BEF7/light/state"
      state_value_template: "{{ value_json.state }}"
      command_topic: "homeassistant/espnow_led/70039F44BEF7/light/set"
      brightness_state_topic: "homeassistant/espnow_led/70039F44BEF7/light/state"
      brightness_value_template: "{{ value_json.brightness }}"
      brightness_command_topic: "homeassistant/espnow_led/70039F44BEF7/light/brightness"
      rgb_state_topic: "homeassistant/espnow_led/70039F44BEF7/light/state"
      rgb_value_template: "{{ value_json.rgb | join(',') }}"
      rgb_command_topic: "homeassistant/espnow_led/70039F44BEF7/light/rgb"
      color_temp_state_topic: "homeassistant/espnow_led/70039F44BEF7/light/state"
      color_temp_value_template: "{{ value_json.temperature }}"
      color_temp_command_topic: "homeassistant/espnow_led/70039F44BEF7/light/temperature"
      json_attributes_topic: "homeassistant/espnow_led/70039F44BEF7/attributes"
      availability:
        - topic: "homeassistant/espnow_led/70039F44BEF7/status"
      payload_on: "ON"
      payload_off: "OFF"
      optimistic: false
      qos: 2
      retain: true

## Версии:

1. v1.0 Начальная версия.