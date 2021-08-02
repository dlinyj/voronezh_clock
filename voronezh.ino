#include <LiquidCrystal.h>
#include <TimeLib.h>

tmElements_t m_time = {0};
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);// RS E

void setup() {
	Serial.begin(9600);
	lcd.begin(16, 2);
}

int settime () {
	int set = 0;
	int keyAnalog =  analogRead(A0);
	delay(50); //Timer 0

	if ((keyAnalog < 800) && (keyAnalog > 600)) { //select = reset
		set++;
		m_time = {0};
		goto exit;
	}
	if (keyAnalog < 100) { //right
		set++;
		m_time.Minute++; //m++;
		if (m_time.Minute > 59) m_time.Minute = 0;
		goto exit;
	}	
	if ((keyAnalog < 600) && (keyAnalog > 400)) { //left
		set++;
		if (m_time.Minute == 0) {
			m_time.Minute = 59;
		} else {
			m_time.Minute--;
		}
		goto exit;
	}
	
	if ((keyAnalog < 200) && (keyAnalog > 130)) { //up
		m_time.Hour++; //h++;
		if (m_time.Hour > 23) m_time.Hour = 0;
		goto exit;
	} 

	if ((keyAnalog < 400) && (keyAnalog > 200)) { //down
		set++;
		if (m_time.Hour == 0) {
			m_time.Hour = 23;
		} else {
			m_time.Hour--;
		}
		goto exit;
	}
exit:
	if (set) {
		m_time.Second = 0;
	}
	delay(200);
	return set;
}

int calc_of_the_current_time () {
	static unsigned long previousMillis = 0;  
	const long interval = 1000;
	unsigned long currentMillis = millis();
	if(currentMillis - previousMillis > interval) {
		previousMillis = currentMillis;
		m_time.Second++;    
		if (m_time.Second > 59) {   // если значение s больше 59
			m_time.Second = 0;       // присваиваем значение 0 переменной s
			m_time.Minute++;     // добавляем 1 к переменной m отвечающей за минуты
		}
		// секция подсчета минут
		if (m_time.Minute > 59) {   // если значение m больше 59
			m_time.Minute = 0;       // присваиваем значение 0 переменной m
			m_time.Hour++;     // добавляем 1 к переменной h отвечающей за часы
		}
		// секция подсчета часов
		if (m_time.Hour  > 23) {  // если значение h больше 23
			m_time.Hour  = 0;       // присваиваем значение 0 переменной h
		}
		return 1;
	} else {
		return 0;
	}
}

void print_the_time () {
	if (m_time.Second < 10) { //если секунд меньше 10
		lcd.setCursor(6, 0); // курсор в 6 позицию 0 строки
		lcd.print(0); //печатаем 0
		lcd.setCursor(7, 0); //курсор в 7 позицию 0 строки
		lcd.print(m_time.Second); //печатаем значение переменной s
	}	else {
		lcd.setCursor(6, 0); //иначе курсор в 6 позицию 0 строки
		lcd.print(m_time.Second); // печатаем значение переменной s
	}
		lcd.setCursor(5, 0); // курсор в 5 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между секундами и минутами
			// вывод минут
	if (m_time.Minute < 10) {
			lcd.setCursor(3, 0);
			lcd.print(0);
			lcd.setCursor(4, 0);
			lcd.print(m_time.Minute);
	} else {
		lcd.setCursor(3, 0);
		lcd.print(m_time.Minute);
	}
		lcd.setCursor(2, 0); // курсор в 2 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между минутами и часами
		// вывод часов
	if (m_time.Hour < 10) {
		lcd.setCursor(0, 0);
		lcd.print(0);
		lcd.setCursor(1, 0);
		lcd.print(m_time.Hour);
	} else {
		lcd.setCursor(0, 0);
		lcd.print(m_time.Hour);
	}
}

void loop() {
	if ( settime() || calc_of_the_current_time ()) {
		print_the_time ();
	}
}
