#include <Arduino.h>
#include <SoftwareSerial.h>
#include <FastBot2.h>
#include <ups.h>
#include <secrets.h>

FastBot2 bot;
SoftwareSerial upsSerial(4, 5);
UPS* ups = nullptr;

String formatFloat(float value, int decimals = 1) {
  if (value == (int)value) return String((int)value);

  char buffer[16];
  sprintf(buffer, "%.*f", decimals, value);
  return String(buffer);
}

String outGridData(UPSGridStatus data) {
  if (data.isConnected) {
    return String(data.voltage) + " В (" + formatFloat(data.frequency) + " Гц)";
  } else {
    return "Отсутствует";
  }
}

void updateh(fb::Update& u) {
  if (!u.isMessage()) return;

  const auto chatId = u.message().chat().id();
  if (chatId != USER_ID) return;

  const String& text = u.message().text();

  if (text == "/start") {
    bot.sendMessage(fb::Message("ℹ Используйте команду /status", chatId));
  } else if (text == "/status") {
    UPSStatus data = ups->getStatus();

    if (!data.isConnected) {
      bot.sendMessage(fb::Message("⚠️ Нет связи с ИБП", chatId));
      return;
    }

    String message = "💡 Нагрузка: " + String(data.load.watt) + " Вт (" + String(data.load.percent) + "%)\n";
    message += "🔋 Заряд батареи: ~" + String(data.battery.capacity) + "% (" + formatFloat(data.battery.voltage) + " В)\n";
    message += "🌡️ Температура корпуса: " + String(data.temperature) + "°С\n";
    message += "🔌 Напряжение сети: " + outGridData(data.grid) + "\n";
    message += "⚡️ Выходное напряжение: " + String(data.output.voltage) + " В (" + formatFloat(data.output.frequency) + " Гц)";
    bot.sendMessage(fb::Message(message, chatId));
  } else if (text == "/update") {
    bot.sendMessage(fb::Message("🔄 Отправьте файл прошивки адаптера в формате .bin с командой /update", chatId));
  } else if (u.message().hasDocument() && u.message().caption() == "/update" && u.message().document().name().endsWith(".bin")) {
    bot.sendMessage(fb::Message("🔄 Обновление прошивки адаптера...", chatId));
    fb::Fetcher fetch = bot.downloadFile(u.message().document().id());
    bool ok = fetch.updateFlash();
    bot.sendMessage(fb::Message(ok ? "✅ Прошивка адаптера успешно обновлена" : "❌ Ошибка обновления прошивки адаптера", chatId));
  } else {
    bot.sendMessage(fb::Message("❌ Неизвестная команда", chatId));
  }
}

void setup() {
  ups = new UPS(upsSerial);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  bot.attachUpdate(updateh);
  bot.setToken(F(BOT_TOKEN));
  bot.setPollMode(fb::Poll::Long, 20000);

  ups->onConnect([]() {
    bot.sendMessage(fb::Message("🔌Подключение к ИБП установлено", USER_ID));
  });

  ups->onDisconnect([]() {
    bot.sendMessage(fb::Message("⚠️ Потеря связи с ИБП", USER_ID));
  });

  ups->onGridConnect([]() {
    bot.sendMessage(fb::Message("🔌Подключение к сети восстановлено", USER_ID));
  });

  ups->onGridDisconnect([]() {
    bot.sendMessage(fb::Message("⚠️ Пропадание сетевого питания! Устройство работает от батареи", USER_ID));
  });

  ups->onBatteryCharged([](float capacity) {
    bot.sendMessage(fb::Message("🔋 Заряд батареи восстановлен. Текущий заряд: " + String((int)capacity) + "%", USER_ID));
  });

  ups->onBatteryDischarged([](float capacity) {
    bot.sendMessage(fb::Message("⚠️ Заряд батареи низкий! Осталось: " + String((int)capacity) + "%", USER_ID));
  });
}

void loop() {
  static bool firstCall = true;
  static unsigned long lastUpsTick = 0;
  unsigned long currentMillis = millis();
  
  if (firstCall) {
    ups->tick();
    firstCall = false;
    lastUpsTick = currentMillis;
  } else if (currentMillis - lastUpsTick >= 5000) {
    ups->tick();
    lastUpsTick = currentMillis;
  }
  
  bot.tick();
}
