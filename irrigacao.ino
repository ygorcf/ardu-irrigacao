#include <Arduino.h>
#include <AceTime.h>

using namespace ace_time;

typedef struct {
  LocalDateTime dateTime;
  unsigned long lastMillis;
} LocalData;

LocalData *localData;

uint8_t buttons[] = {2, 3, 4, 5};

#define BUTTON_UP 1
#define BUTTON_RIGHT 2
#define BUTTON_DONW 3
#define BUTTON_LEFT 4

int relePin = 6;
int INTERVAL_TIME_MIN = 16;
int OPEN_TIME_MIN = 16;
unsigned long intervalTime;
unsigned long openTime;
unsigned long time;

void setup() {
    Serial.begin(9600);
    
    for (int i = 0; i < 4; i++) {
      pinMode(buttons[i], INPUT_PULLUP);
    }
    pinMode(relePin, OUTPUT);

    localData = (LocalData *) malloc(sizeof(LocalData));
    localData->lastMillis = millis();
    localData->dateTime = LocalDateTime::forComponents(2022, 12, 20, 0, 0, 0);

    intervalTime = minutes(INTERVAL_TIME_MIN);
    openTime = minutes(OPEN_TIME_MIN);
}

void loop() {
    int buttonPressed = getButtonPressed();
    drawMenu();

    if (!passedTime(1000)) {
      return;
    }

    updateTime();
}

void toggleRele(bool value) {
    digitalWrite(relePin, value ? LOW : HIGH);
    // analogWrite(relePin, value ? 255 : 0);
}

unsigned long minutes(long min) {
    unsigned long oneSec = 1000;
    unsigned long oneMin = oneSec;
    return min * oneMin;
}

void updateTime() {
    localData->lastMillis = millis();
    localData->dateTime.second(localData->dateTime.second() + 1);

    if (localData->dateTime.second() >= 60) {
        localData->dateTime.minute(localData->dateTime.minute() + 1);
        localData->dateTime.second(0);
    }

    if (localData->dateTime.minute() >= 60) {
        localData->dateTime.hour(localData->dateTime.hour() + 1);
        localData->dateTime.minute(0);
    }

    if (localData->dateTime.hour() >= 24) {
        localData->dateTime.day(localData->dateTime.day() + 1);
        localData->dateTime.hour(0);
    }

    if (localData->dateTime.day() >= max_days_on(localData->dateTime.month()) + 1) {
        localData->dateTime.month(localData->dateTime.month() + 1);
        localData->dateTime.day(0);
    }

    if (localData->dateTime.month() >= 13) {
        localData->dateTime.year(localData->dateTime.year() + 1);
        localData->dateTime.month(1);
    }

    if (localData->dateTime.year() >= 10000) {
        localData->dateTime.year(0);
    }
}

uint8_t max_days_on(uint8_t month) {
    switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        case 2:
            return 28;
        default:
            return 30;
    }
}

boolean passedTime(unsigned long time) {
  return millis() - localData->lastMillis >= time;
}

byte getButtonPressed() {
  for (byte i = 0; i < 4; i++) {
    if (digitalRead(buttons[i]) == LOW) {
      return i + 1;
    }
  }

  return 0;
}

void drawMenu() {
  char dateLine[36];
  sprintf(
    dateLine,
    "| Hora: %2d/%2d/%2d %02d:%02d:%02d  |\0",
    localData->dateTime.day(),
    localData->dateTime.month(),
    localData->dateTime.year(),
    localData->dateTime.hour(),
    localData->dateTime.minute(),
    localData->dateTime.second()
  );

  char *lines[] = {
    "|         ArduIrriga         |",
    "|----------------------------|",
    dateLine,
    "|----------------------------|"
  };
  

  for (int i = 0; i < 4; i++) {
    Serial.println((lines[i]));
  }
  Serial.println("\n\n\n\n\n\n\n\n");
}

uint8_t strLen(char *str) {
  uint8_t len = 0;

  if (str == NULL) {
    return 0;
  }

  while (str[len] != '\0') {
    len++;
  }

  return len;
}
