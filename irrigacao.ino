#include <Arduino.h>
#include <AceTime.h>
#include <EEPROM.h>
#include "U8glib.h"

#define BACK_ID 0
#define CHANGE_DATETIME_ID 1
#define CHANGE_DATETIME_DAY_ID 2
#define CHANGE_DATETIME_MONTH_ID 3
#define CHANGE_DATETIME_YEAR_ID 4
#define CHANGE_DATETIME_HOUR_ID 5
#define CHANGE_DATETIME_MIN_ID 6
#define CHANGE_DATETIME_SEC_ID 7
#define CHANGE_IRRIGATION_TIME_OFF_ID 8
#define CHANGE_IRRIGATION_TIME_OFF_HOUR_ID 9
#define CHANGE_IRRIGATION_TIME_OFF_MIN_ID 10
#define CHANGE_IRRIGATION_TIME_OFF_SEC_ID 11
#define CHANGE_IRRIGATION_TIME_ON_ID 12
#define CHANGE_IRRIGATION_TIME_ON_HOUR_ID 13
#define CHANGE_IRRIGATION_TIME_ON_MIN_ID 14
#define CHANGE_IRRIGATION_TIME_ON_SEC_ID 15
#define CHANGING_DATETIME_ID 16

#define DATE_TYPE_YEAR 0
#define DATE_TYPE_MONTH 1
#define DATE_TYPE_HOUR 2
#define DATE_TYPE_MINUTE 3
#define DATE_TYPE_SECOND 4

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

using namespace ace_time;

typedef struct MenuOptionType {
  uint8_t id;
  char *label;
  struct MenuOptionType *child;
  struct MenuOptionType *next;
  struct MenuOptionType *previous;
} MenuOption;

typedef struct {
  LocalDateTime dateTime;
  unsigned long lastMillis;
  MenuOption *option;
} LocalData;

LocalData *localData;

uint8_t buttons[] = {2, 3, 4, 5};

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

    MenuOption *changeTimeDayOption = createOption(CHANGE_DATETIME_DAY_ID, "Alterar Dia", NULL, NULL, NULL);
    MenuOption *changeTimeMonthOption = createOption(CHANGE_DATETIME_MONTH_ID, "Alterar Mes", NULL, NULL, changeTimeDayOption);
    MenuOption *changeTimeYearOption = createOption(CHANGE_DATETIME_YEAR_ID, "Alterar Ano", NULL, NULL, changeTimeMonthOption);
    MenuOption *changeTimeHourOption = createOption(CHANGE_DATETIME_HOUR_ID, "Alterar Hora", NULL, NULL, changeTimeYearOption);
    MenuOption *changeTimeMinOption = createOption(CHANGE_DATETIME_MIN_ID, "Alterar Min", NULL, NULL, changeTimeHourOption);
    MenuOption *changeTimeSecOption = createOption(CHANGE_DATETIME_SEC_ID, "Alterar Seg", NULL, NULL, changeTimeMinOption);
    MenuOption *changeTimeBackOption = createOption(BACK_ID, "Voltar", changeTimeOption, NULL, changeTimeSecOption);
    changeTimeDayOption->previous = changeTimeBackOption;
    changeTimeDayOption->next = changeTimeMonthOption;
    changeTimeMonthOption->next = changeTimeYearOption;
    changeTimeYearOption->next = changeTimeHourOption;
    changeTimeHourOption->next = changeTimeMinOption;
    changeTimeMinOption->next = changeTimeSecOption;
    changeTimeSecOption->next = changeTimeBackOption;
    changeTimeBackOption->next = changeTimeDayOption;

    MenuOption *changeIrrigationOffTimeOption = createOption(CHANGE_IRRIGATION_TIME_OFF_ID, "Alt Irri Des", NULL, NULL, changeTimeOption);

    MenuOption *changeIrrigationOffTimeHourOption = createOption(CHANGE_IRRIGATION_TIME_OFF_HOUR_ID, "Alterar Hora", NULL, NULL, NULL);
    MenuOption *changeIrrigationOffTimeMinOption = createOption(CHANGE_IRRIGATION_TIME_OFF_MIN_ID, "Alterar Min", NULL, NULL, changeIrrigationOffTimeHourOption);
    MenuOption *changeIrrigationOffTimeSecOption = createOption(CHANGE_IRRIGATION_TIME_OFF_SEC_ID, "Alterar Seg", NULL, NULL, changeIrrigationOffTimeMinOption);
    MenuOption *changeIrrigationOffTimeBackOption = createOption(BACK_ID, "Voltar", changeIrrigationOffTimeOption, NULL, changeIrrigationOffTimeSecOption);
    changeIrrigationOffTimeHourOption->previous = changeIrrigationOffTimeBackOption;
    changeIrrigationOffTimeHourOption->next = changeIrrigationOffTimeMinOption;
    changeIrrigationOffTimeMinOption->next = changeIrrigationOffTimeSecOption;
    changeIrrigationOffTimeSecOption->next = changeIrrigationOffTimeBackOption;
    changeIrrigationOffTimeBackOption->next = changeIrrigationOffTimeHourOption;

    MenuOption *changeIrrigationOnTimeOption = createOption(CHANGE_IRRIGATION_TIME_ON_ID, "Alt Irri Lig", NULL, NULL, changeIrrigationOffTimeOption);

    MenuOption *changeIrrigationOnTimeHourOption = createOption(CHANGE_IRRIGATION_TIME_ON_HOUR_ID, "Alterar Hora", NULL, NULL, NULL);
    MenuOption *changeIrrigationOnTimeMinOption = createOption(CHANGE_IRRIGATION_TIME_ON_MIN_ID, "Alterar Min", NULL, NULL, changeIrrigationOnTimeHourOption);
    MenuOption *changeIrrigationOnTimeSecOption = createOption(CHANGE_IRRIGATION_TIME_ON_SEC_ID, "Alterar Seg", NULL, NULL, changeIrrigationOnTimeMinOption);
    MenuOption *changeIrrigationOnTimeBackOption = createOption(BACK_ID, "Voltar", changeIrrigationOnTimeOption, NULL, changeIrrigationOnTimeSecOption);
    changeIrrigationOnTimeHourOption->previous = changeIrrigationOnTimeBackOption;
    changeIrrigationOnTimeHourOption->next = changeIrrigationOnTimeMinOption;
    changeIrrigationOnTimeMinOption->next = changeIrrigationOnTimeSecOption;
    changeIrrigationOnTimeSecOption->next = changeIrrigationOnTimeBackOption;
    changeIrrigationOnTimeBackOption->next = changeIrrigationOnTimeHourOption;

    changeTimeOption->child = changeTimeDayOption;
    changeTimeOption->next = changeIrrigationOffTimeOption;
    changeTimeOption->previous = changeIrrigationOnTimeOption;
    changeIrrigationOffTimeOption->child = changeIrrigationOffTimeHourOption;
    changeIrrigationOffTimeOption->next = changeIrrigationOnTimeOption;
    changeIrrigationOnTimeOption->child = changeIrrigationOnTimeHourOption;
    changeIrrigationOnTimeOption->next = changeTimeOption;

    localData = (LocalData *) malloc(sizeof(LocalData));
    localData->lastMillis = millis();
    localData->dateTime = loadDateSaved();
    localData->option = changeTimeOption;

    intervalTime = minutes(INTERVAL_TIME_MIN);
    openTime = minutes(OPEN_TIME_MIN);

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
    addToDateTime(&localData->dateTime, 1);
    saveDate(&localData->dateTime);
}

void addToDateTime(LocalDateTime *dateTime, long addSeconds) {
  uint8_t secondsAdd = addSeconds;
  while(secondsAdd > 0) {
    localData->dateTime.second(localData->dateTime.second() + 1);
    secondsAdd--;

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

    if (localData->dateTime.day() >= maxDaysOn(localData->dateTime.month()) + 1) {
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
}

int maxDateTypeOn(int id, LocalDateTime *dateTime) {
  int type;
  switch (id) {
    case CHANGE_DATETIME_SEC_ID:
      type = DATE_TYPE_SECOND;
      break;
    case CHANGE_DATETIME_MIN_ID:
      type = DATE_TYPE_MINUTE;
      break;
    case CHANGE_DATETIME_HOUR_ID:
      type = DATE_TYPE_HOUR;
      break;
    case CHANGE_DATETIME_DAY_ID:
      return maxDaysOn(dateTime->month());
    case CHANGE_DATETIME_MONTH_ID:
      type = DATE_TYPE_MONTH;
      break;
    case CHANGE_DATETIME_YEAR_ID:
      type = DATE_TYPE_YEAR;
      break;
  }

  return maxDateType(type);
}

int maxDateType(uint8_t dateType) {
  switch (dateType) {
    case DATE_TYPE_YEAR:
      return 10000;
    case DATE_TYPE_MONTH:
      return 13;
    case DATE_TYPE_HOUR:
      return 24;
    case DATE_TYPE_MINUTE:
      return 60;
    case DATE_TYPE_SECOND:
      return 60;
    default:
      return 0;
  }
}

uint8_t maxDaysOn(uint8_t month) {
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

void handleButtonPressed(byte buttonPressed) {
  switch (buttonPressed) {
    case BUTTON_UP:
      if (localData->option->id == CHANGING_DATETIME_ID) {
        handleChangingTime(1);
      } else {
        localData->option = localData->option->previous;
      }
      break;
    case BUTTON_DOWN:
      if (localData->option->id == CHANGING_DATETIME_ID) {
        handleChangingTime(-1);
      } else {
        localData->option = localData->option->next;
      }
      break;
    case BUTTON_OK:
      handleSelectOption();
      break;
  }
}

void handleSelectOption() {
  if (localData->option->id == CHANGE_DATETIME_SEC_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.second(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_MIN_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.minute(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_HOUR_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.hour(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_DAY_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.day(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_MONTH_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.month(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_YEAR_ID) {
    localData->option = createMenuOptionForUpdate(localData->dateTime.year(), localData->option);
  } else if (localData->option->id == CHANGE_DATETIME_ID ||
    localData->option->id == BACK_ID ||
    localData->option->id == CHANGE_IRRIGATION_TIME_OFF_ID ||
    localData->option->id == CHANGE_IRRIGATION_TIME_ON_ID) {
    localData->option = localData->option->child;
  } else if (localData->option->id == CHANGING_DATETIME_ID) {
    handleChangedDateTimeSelected();
    localData->option = localData->option->child;
  }
}

void handleChangedDateTimeSelected() {
  int value = atoi(localData->option->label);
  switch (localData->option->child->id) {
    case CHANGE_DATETIME_SEC_ID:
      localData->dateTime.second(value);
      break;
    case CHANGE_DATETIME_MIN_ID:
      localData->dateTime.minute(value);
      break;
    case CHANGE_DATETIME_HOUR_ID:
      localData->dateTime.hour(value);
      break;
    case CHANGE_DATETIME_DAY_ID:
      localData->dateTime.day(value);
      break;
    case CHANGE_DATETIME_MONTH_ID:
      localData->dateTime.month(value);
      break;
    case CHANGE_DATETIME_YEAR_ID:
      localData->dateTime.year(value);
      break;
  }
}

void handleChangingTime(int valueAdd) {
  char *label = (char *) malloc(6 * sizeof(char));
  int labelInt = atoi(localData->option->label) + valueAdd;
  int maxValue = maxDateTypeOn(localData->option->child->id, &localData->dateTime);

  if (labelInt >= maxValue) {
    labelInt = labelInt - maxValue;
  }
  if (labelInt < 0) {
    labelInt = 0;
  }

  sprintf(label, "%5d\0", labelInt);
  localData->option->label = label;
}

void draw() {
    u8g.firstPage();  
    do {
        drawMenu();
    } while( u8g.nextPage() );
}

void drawMenu() {
  char dateLine[36];
  sprintf(
    dateLine,
    "Hora: %02d/%02d/%02d %02d:%02d:%02d\0",
    localData->dateTime.day(),
    localData->dateTime.month(),
    localData->dateTime.year(),
    localData->dateTime.hour(),
    localData->dateTime.minute(),
    localData->dateTime.second()
  );

  //Comandos graficos para o display devem ser colocados aqui
  //Seleciona a fonte de texto
  u8g.setFont(u8g_font_8x13B);
  //Linha superior - temperatura 
  u8g.drawStr(27, 12, "ArduIrriga");
  //Hora
  u8g.setFont(u8g_font_5x8);
  u8g.drawStr(3, 28, dateLine);
  
  u8g.setFont(u8g_font_unifont);
  u8g.drawStr(5, 48, localData->option->label);
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

MenuOption *createMenuOptionForUpdate(int value, MenuOption *lastOption) {
    char *label = (char *) malloc(6 * sizeof(char));
    sprintf(label, "%5d\0", value);
    return createOption(CHANGING_DATETIME_ID, label, lastOption, NULL, NULL);
}

LocalDateTime loadDateSaved() {
  int year = readFromSaved(0, 2022);
  int month = readFromSaved(1 * sizeof(int), 12);
  int day = readFromSaved(2 * sizeof(int), 30);
  int hour = readFromSaved(3 * sizeof(int), 0);
  int min = readFromSaved(4 * sizeof(int), 0);
  int sec = readFromSaved(5 * sizeof(int), 0);

  return LocalDateTime::forComponents(
    year,
    month,
    day,
    hour,
    min,
    sec);
}

int readFromSaved(int address, int defaultValue) {
  int value = 0;
  EEPROM.get(address, value);

  if (value == 0) {
    value = defaultValue;
  }

  return value;
}

void saveDate(LocalDateTime *dateTime) {
  EEPROM.put(0, dateTime->year());
  EEPROM.put(1 * sizeof(int), dateTime->month());
  EEPROM.put(2 * sizeof(int), dateTime->day());
  EEPROM.put(3 * sizeof(int), dateTime->hour());
  EEPROM.put(4 * sizeof(int), dateTime->minute());
  EEPROM.put(5 * sizeof(int), dateTime->second());
}
