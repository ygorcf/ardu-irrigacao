int relePin = 6;
int INTERVAL_TIME_MIN = 16;
int OPEN_TIME_MIN = 16;
unsigned long intervalTime;
unsigned long openTime;
unsigned long time;

void setup() {
    Serial.begin(9600);
    pinMode(relePin, OUTPUT);

    intervalTime = minutes(INTERVAL_TIME_MIN);
    openTime = minutes(OPEN_TIME_MIN);
}

void loop() {
    time = millis();
    toggleRele(false);
    Serial.println("rele: False");
    Serial.print("Time: ");
    Serial.println(time);
    delay(intervalTime);

    time = millis();
    toggleRele(true);
    Serial.println("rele: True");
    Serial.print("Time: ");
    Serial.println(time);
    delay(openTime);
}

void toggleRele(bool value) {
    digitalWrite(relePin, value ? LOW : HIGH);
    // analogWrite(relePin, value ? 255 : 0);
}

unsigned long minutes(long min) {
    unsigned long oneSec = 1000;
    unsigned long oneMin = 60 * oneSec;
    return min * oneMin;
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
