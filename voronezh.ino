#include <LiquidCrystal.h>

int h,m,s;// переменные для часов, минут, секунд
unsigned long previousMillis = 0;  
const long interval = 1000;
int keyAnalog;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);// RS E


void setup() {
	Serial.begin(9600);
	lcd.begin(16, 2);
}

int settime () {
int set = 0;
	keyAnalog =  analogRead(A0);
	delay(50); //Timer 0
	if (keyAnalog < 100) { //right
		set++;
		m++;
		if (m > 59) m = 0;
		goto exit;
	}	
	if ((keyAnalog < 200) && (keyAnalog > 130)) { //up
		s = 0;
		h++;
		if (h > 23) h = 0;
		goto exit;
	} 
	if ((keyAnalog < 800) && (keyAnalog > 600)) { //select = reset
		set++;
		m = 0;
		h = 0;
		goto exit;
	}
	if ((keyAnalog < 600) && (keyAnalog > 400)) { //left
		set++;
		m--;
		if (m < 0) m = 59;
		goto exit;
	}	
	if ((keyAnalog < 400) && (keyAnalog > 200)) { //down
		set++;
		h--;
		if (h < 0) h = 23;
		goto exit;
	}
exit:
	if (set) {
		s = 0;
	}
	delay(200);
	return set;
}

void loop() {
	settime();
	unsigned long currentMillis = millis();
	if(currentMillis - previousMillis > interval) {
		previousMillis = currentMillis;
		s++;    // добавляем единицу, равносильно записи s=s+1;
		// секция подсчета секунд
		if (s > 59) {   // если значение s больше 59
			s = 0;       // присваиваем значение 0 переменной s
			m++;     // добавляем 1 к переменной m отвечающей за минуты
		}
		// секция подсчета минут
		if (m > 59) {   // если значение m больше 59
			m = 0;       // присваиваем значение 0 переменной m
			h++;     // добавляем 1 к переменной h отвечающей за часы
		}
		// секция подсчета часов
		if (h > 23) {  // если значение h больше 23
			h = 0;       // присваиваем значение 0 переменной h
		}
		// секция вывода информации
		// вывод секунд
		if (s<10) { //если секунд меньше 10
			lcd.setCursor(6, 0); // курсор в 6 позицию 0 строки
			lcd.print(0); //печатаем 0
			lcd.setCursor(7, 0); //курсор в 7 позицию 0 строки
			lcd.print(s); //печатаем значение переменной s
		}	else {
			lcd.setCursor(6, 0); //иначе курсор в 6 позицию 0 строки
			lcd.print(s); // печатаем значение переменной s
		}
		lcd.setCursor(5, 0); // курсор в 5 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между секундами и минутами
			// вывод минут
		if (m<10) {
			lcd.setCursor(3, 0);
			lcd.print(0);
			lcd.setCursor(4, 0);
			lcd.print(m);
		} else {
			lcd.setCursor(3, 0);
			lcd.print(m);
		}
		lcd.setCursor(2, 0); // курсор в 2 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между минутами и часами
		// вывод часов
		if (h<10) {
			lcd.setCursor(0, 0);
			lcd.print(0);
			lcd.setCursor(1, 0);
			lcd.print(h);
		} else {
			lcd.setCursor(0, 0);
			lcd.print(h);
		}
	}
}
