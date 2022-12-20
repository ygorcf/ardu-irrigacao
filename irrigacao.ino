#include <Arduino.h>
#include <AceTime.h>

using namespace ace_time;

int relePin = 6;
int INTERVAL_TIME_MIN = 16;
int OPEN_TIME_MIN = 16;
unsigned long intervalTime;
unsigned long openTime;
unsigned long time;
unsigned long lastSecMillis;
LocalDateTime dateTime;


void setup() {
    Serial.begin(9600);
    pinMode(relePin, OUTPUT);

    intervalTime = minutes(INTERVAL_TIME_MIN);
    openTime = minutes(OPEN_TIME_MIN);
    dateTime = LocalDateTime::forComponents(2022, 12, 20, 0, 0, 0);
    lastSecMillis = millis();
}

void loop() {
    updateTime();
    dateTime.printTo(Serial);
    Serial.println();
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
    if ((lastSecMillis + 1) < millis()) {
        lastSecMillis = millis();
        dateTime.second(dateTime.second() + 1);

        if (dateTime.second() >= 60) {
            dateTime.minute(dateTime.minute() + 1);
            dateTime.second(0);
        }

        if (dateTime.minute() >= 60) {
            dateTime.hour(dateTime.hour() + 1);
            dateTime.minute(0);
        }

        if (dateTime.hour() >= 24) {
            dateTime.day(dateTime.day() + 1);
            dateTime.hour(0);
        }

        if (dateTime.day() >= max_days_on(dateTime.month()) + 1) {
            dateTime.month(dateTime.month() + 1);
            dateTime.day(0);
        }

        if (dateTime.month() >= 13) {
            dateTime.year(dateTime.year() + 1);
            dateTime.month(1);


        }

        if (dateTime.year() >= 10000) {
            dateTime.year(0);
        }
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

/*
v1
08m - 1
16m - 2
32m - 4
01h - 8m
02h - 16m
04h - 32m
08h - 1h
12h - 1,5h

15m - 1
30m - 2
01h - 4
02h - 8
04h - 16m
08h - 30m
12h - 45m

30m - 1
1h - 2
2h - 4
4h - 8
8h - 16m
*/
