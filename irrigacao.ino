#include <Arduino.h>
#include <EEPROM.h>
#include "U8glib.h"

#define BACK_ID 0
#define CHANGE_DATETIME_ID 1
#define CHANGE_DATETIME_HOUR_ID 2
#define CHANGE_DATETIME_MIN_ID 3
#define CHANGE_DATETIME_SEC_ID 4
#define CHANGE_IRRIGATION_TIME_OFF_ID 5
#define CHANGE_IRRIGATION_TIME_ON_ID 6
#define CHANGING_DATETIME_ID 7

#define DATE_TYPE_HOUR 0
#define DATE_TYPE_MINUTE 1
#define DATE_TYPE_SECOND 2

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

typedef struct MenuOptionType {
  uint8_t id;
  char *label;
  struct MenuOptionType *child;
  struct MenuOptionType *next;
  struct MenuOptionType *previous;
} MenuOption;

typedef struct {
  uint8_t value;
} TimeValue;

typedef struct {
  TimeValue hour;
  TimeValue minute;
  TimeValue second;
} TimeT;

typedef struct {
  TimeValue *changingValue;
  uint8_t changeBy;
  uint8_t maxValue;
  MenuOption *backOption;
} ChangingOption;

typedef struct {
  TimeT currentTime;
  unsigned long lastMillis;
  MenuOption *option;
  ChangingOption *changingOption;
} LocalData;

LocalData *localData;

uint8_t buttons[] = {2, 3, 4};

#define BUTTON_UP 1
#define BUTTON_DOWN 2
#define BUTTON_OK 3

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

    MenuOption *changeTimeOption = createOption(CHANGE_DATETIME_ID, "Alterar Horas", NULL, NULL, NULL);

    MenuOption *changeTimeHourOption = createOption(CHANGE_DATETIME_HOUR_ID, "Alterar Hora", NULL, NULL, NULL);
    MenuOption *changeTimeMinOption = createOption(CHANGE_DATETIME_MIN_ID, "Alterar Min", NULL, NULL, changeTimeHourOption);
    MenuOption *changeTimeSecOption = createOption(CHANGE_DATETIME_SEC_ID, "Alterar Seg", NULL, NULL, changeTimeMinOption);
    MenuOption *changeTimeBackOption = createOption(BACK_ID, "Voltar", changeTimeOption, NULL, changeTimeSecOption);
    changeTimeHourOption->previous = changeTimeBackOption;
    changeTimeHourOption->next = changeTimeMinOption;
    changeTimeMinOption->next = changeTimeSecOption;
    changeTimeSecOption->next = changeTimeBackOption;
    changeTimeBackOption->next = changeTimeHourOption;

    MenuOption *changeIrrigationOffTimeOption = createOption(CHANGE_IRRIGATION_TIME_OFF_ID, "Alt Irri Des", NULL, NULL, changeTimeOption);

    MenuOption *changeIrrigationOnTimeOption = createOption(CHANGE_IRRIGATION_TIME_ON_ID, "Alt Irri Lig", NULL, NULL, changeIrrigationOffTimeOption);

    changeTimeOption->child = changeTimeHourOption;
    changeTimeOption->next = changeIrrigationOffTimeOption;
    changeTimeOption->previous = changeIrrigationOnTimeOption;
    changeIrrigationOffTimeOption->next = changeIrrigationOnTimeOption;
    changeIrrigationOnTimeOption->next = changeTimeOption;

    localData = (LocalData *) malloc(sizeof(LocalData));
    localData->lastMillis = millis();
    localData->currentTime = loadDateSaved();
    localData->option = changeTimeOption;

    // old
    intervalTime = minutes(INTERVAL_TIME_MIN);
    openTime = minutes(OPEN_TIME_MIN);

    // setup display
    if (u8g.getMode() == U8G_MODE_R3G3B2) {
      u8g.setColorIndex(255);     // white
    } else if (u8g.getMode() == U8G_MODE_GRAY2BIT) {
      u8g.setColorIndex(3);         // max intensity
    } else if (u8g.getMode() == U8G_MODE_BW) {
      u8g.setColorIndex(1);         // pixel on
    } else if (u8g.getMode() == U8G_MODE_HICOLOR) {
      u8g.setHiColorByRGB(255,255,255);
    }
}

void loop() {
    handleButtonPressed(getButtonPressed());
    
    draw();

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
  if (!isChangingTime()) {
    addToTime(&localData->currentTime, 1);
    saveDate(&localData->currentTime);
  }
}

void addToTime(TimeT *time, long addSeconds) {
  uint8_t secondsAdd = addSeconds;
  while(secondsAdd > 0) {
    time->second.value++;
    secondsAdd--;

    if (time->second.value >= 60) {
        time->minute.value++;
        time->second.value = 0;
    }

    if (time->minute.value >= 60) {
        time->hour.value++;
        time->minute.value = 0;
    }

    if (time->hour.value >= 24) {
        time->hour.value = 0;
    }
  }
}

boolean passedTime(unsigned long time) {
  return millis() - localData->lastMillis >= time;
}

uint8_t getButtonPressed() {
  for (uint8_t i = 0; i < 3; i++) {
    if (digitalRead(buttons[i]) == LOW) {
      return i + 1;
    }
  }

  return 0;
}

void handleButtonPressed(uint8_t buttonPressed) {
  switch (buttonPressed) {
    case BUTTON_UP:
      if (isChangingTime()) {
        addToChangingTime(1);
      } else {
        localData->option = localData->option->previous;
      }
      break;
    case BUTTON_DOWN:
      if (isChangingTime()) {
        addToChangingTime(-1);
      } else {
        localData->option = localData->option->next;
      }
      break;
    case BUTTON_OK:
      if (isChangingTime()) {
        free(localData->changingOption);
        localData->option = localData->changingOption->backOption;
        localData->changingOption = NULL;
      } else {
        handleSelectOption();
      }
      break;
  }
}

void handleSelectOption() {
  if (localData->option->id == CHANGE_DATETIME_SEC_ID) {
    localData->changingOption = createChangingOption(&localData->currentTime.second, 1, 59, localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_MIN_ID) {
    localData->changingOption = createChangingOption(&localData->currentTime.minute, 1, 59, localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_HOUR_ID) {
    localData->changingOption = createChangingOption(&localData->currentTime.hour, 1, 23, localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_ID ||
    localData->option->id == BACK_ID ||
    localData->option->id == CHANGE_IRRIGATION_TIME_OFF_ID ||
    localData->option->id == CHANGE_IRRIGATION_TIME_ON_ID) {
    localData->option = localData->option->child;
  }
}

void addToChangingTime(int8_t valueAdd) {
  int8_t newValue = (valueAdd * localData->changingOption->changeBy) + localData->changingOption->changingValue->value;

  if (newValue > localData->changingOption->maxValue) {
    newValue = 0;
  } else if (newValue < 0) {
    newValue = (valueAdd * localData->changingOption->changeBy) + localData->changingOption->maxValue + 1;
  }

  localData->changingOption->changingValue->value = newValue;
}

void draw() {
    u8g.firstPage();  
    do {
        drawMenu();
    } while( u8g.nextPage() );
}

void drawMenu() {
  char dateLine[22];
  char *optionLabel;
  bool needFreeOption = false;
  sprintf(
    dateLine,
    "Hora: %02d:%02d:%02d\0",
    localData->currentTime.hour.value,
    localData->currentTime.minute.value,
    localData->currentTime.second.value
  );
  
  if (isChangingTime()) {
    needFreeOption = true;
    optionLabel = (char *) malloc(6 * sizeof(char));
    sprintf(
      optionLabel,
      "%02d\0",
      localData->changingOption->changingValue->value
    );
  } else {
    optionLabel = localData->option->label;
  }

  //Comandos graficos para o display devem ser colocados aqui
  //Seleciona a fonte de texto
  u8g.setFont(u8g_font_8x13B);
  //Titulo 
  u8g.drawStr(27, 12, "ArduIrriga");
  //Hora
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr(3, 28, dateLine);
  //Option
  u8g.setFont(u8g_font_unifont);
  u8g.drawStr(5, 48, optionLabel);

  if (needFreeOption) {
    free(optionLabel);
  }
}

MenuOption *createOption(uint8_t id, char *label, MenuOption *child, MenuOption *next, MenuOption *previous) {
  MenuOption *option = (MenuOption *) malloc(sizeof(MenuOption));

  option->id = id;
  option->label = label;
  option->child = child;
  option->next = next;
  option->previous = previous;

  return option;
}

ChangingOption *createChangingOption(TimeValue *value, uint8_t changeBy, uint8_t maxValue, MenuOption *backOption) {
  ChangingOption *option = (ChangingOption *) malloc(sizeof(ChangingOption));

  option->changingValue = value;
  option->changeBy = changeBy;
  option->maxValue = maxValue;
  option->backOption = backOption;
  
  return option;
}

TimeT loadDateSaved() {
  TimeT currentTime;

  currentTime.hour.value = readFromSaved(0);
  currentTime.minute.value = readFromSaved(sizeof(uint8_t));
  currentTime.second.value = readFromSaved(2 * sizeof(uint8_t));

  return currentTime;
}

int readFromSaved(int address) {
  int value = 0;
  EEPROM.get(address, value);

  if (value <= 0) {
    value = 0;
  }

  return value;
}

void saveDate(TimeT *time) {
  EEPROM.put(0, time->hour.value);
  EEPROM.put(sizeof(uint8_t), time->minute.value);
  EEPROM.put(2 * sizeof(uint8_t), time->second.value);
}

bool isChangingTime() {
  return localData->changingOption != NULL;
}
