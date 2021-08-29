#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <inttypes.h>
#include <DS1307RTC.h>
#include <Wire.h>

#define POS_SIG	2
#define NEG_SIG	3

tmElements_t m_time = {0};
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);// RS E

inline void set_zero_sig() __attribute__((always_inline));
inline void set_posi_sig() __attribute__((always_inline));
inline void set_nego_sig() __attribute__((always_inline));

void setupTimer1() {
	noInterrupts();
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	// 20000 Hz (16000000/((99+1)*8))
	OCR1A = 99;
	// CTC
	TCCR1B |= (1 << WGM12);
	// Prescaler 8
	TCCR1B |= (1 << CS11);
	// Output Compare Match A Interrupt Enable
	TIMSK1 |= (1 << OCIE1A);
	interrupts();
}

void setup() {
//	Serial.begin(9600);
	pinMode(POS_SIG, OUTPUT);
	pinMode(NEG_SIG, OUTPUT);
	lcd.begin(16, 2);
	setupTimer1();
}

int settime () {
	tmElements_t l_time = {0};
	int set = 0;
	RTC.read(l_time); //read time from DS3231
	noInterrupts();
	m_time = l_time; //send time to output function
	interrupts();
	//delay(200)
	static unsigned long old_time = 0;
	static unsigned long new_time;
	new_time = millis();
	if ((new_time - old_time) >= 200) {
		old_time = new_time;
		int keyAnalog =  analogRead(A0);
		if ((keyAnalog < 800) && (keyAnalog > 600)) { //select = reset
			set++;
			l_time = {0};
			goto exit;
		}
		if (keyAnalog < 100) { //right
			set++;
			l_time.Minute++; //m++;
			if (l_time.Minute > 59) l_time.Minute = 0;
			goto exit;
		}	
		if ((keyAnalog < 600) && (keyAnalog > 400)) { //left
			set++;
			if (l_time.Minute == 0) {
				l_time.Minute = 59;
			} else {
				l_time.Minute--;
			}
			goto exit;
		}
		
		if ((keyAnalog < 200) && (keyAnalog > 130)) { //up
			set++;
			l_time.Hour++; //h++;
			if (l_time.Hour > 23) l_time.Hour = 0;
			goto exit;
		} 

		if ((keyAnalog < 400) && (keyAnalog > 200)) { //down
			set++;
			if (l_time.Hour == 0) {
				l_time.Hour = 23;
			} else {
				l_time.Hour--;
			}
			goto exit;
		}
	exit:
		if (set) {
			l_time.Second = 0;
			noInterrupts();
			m_time = l_time; //send time to timer function
			interrupts();
			return RTC.write(l_time);
		}
		return set;
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
	settime();
	print_the_time ();
}

#define PULSE_DURATION	50
#define TIMER_PERIOD	50
#define DELAY_AFTER_PULSE_TRAIN	2700

#define H_SYNC_PULSE	0
#define L_SYNC_PULSE	1
#define NUM_SENDING	2 
#define POS_SIG_NUM	3
#define ZER_SIG_NUM	4

#define SET_BIT(p,n) (p |= (1 << n))
#define CLR_BIT(p,n) (p &= ~(1 << n))
#define CHK_BIT(p,n) (p & (1 << n))


void set_zero_sig() {
	digitalWrite(POS_SIG, HIGH);
	digitalWrite(NEG_SIG, HIGH);
}

void set_posi_sig() {
	digitalWrite(POS_SIG, LOW);
}

void set_nego_sig() {
	digitalWrite(NEG_SIG, LOW);
}

ISR(TIMER1_COMPA_vect) {
	static uint8_t numbers[6] 	={0};
	static uint8_t number_send	= 0;
	static uint8_t current_num	= 0;
	static uint8_t pkg_stage	= 0;
	static uint16_t _delay	 	= 0;
	
	if (_delay >= TIMER_PERIOD) {
		_delay-=TIMER_PERIOD;
		goto exit_isr;
	} else {
		_delay = 0;
	}
	if (0 == pkg_stage) {
		SET_BIT(pkg_stage, H_SYNC_PULSE); //h-sync pulse stage
		numbers[0]	= m_time.Hour / 10;
		numbers[1]	= m_time.Hour % 10;
		numbers[2]	= m_time.Minute / 10;
		numbers[3]	= m_time.Minute % 10;
		numbers[4]	= m_time.Second /10;
		numbers[5]	= m_time.Second % 10;
		//first pulse
		set_nego_sig();
		_delay 		= PULSE_DURATION;
		goto exit_isr;
	} else {
		set_zero_sig();
		if (CHK_BIT (pkg_stage, H_SYNC_PULSE)) {
			CLR_BIT(pkg_stage, H_SYNC_PULSE);
			SET_BIT(pkg_stage, L_SYNC_PULSE); //l-sync pulse stage
			set_posi_sig();
			_delay = PULSE_DURATION;
			goto exit_isr;
		}
		if (CHK_BIT (pkg_stage, L_SYNC_PULSE)) {
			CLR_BIT(pkg_stage, L_SYNC_PULSE);
			SET_BIT(pkg_stage, NUM_SENDING);
			set_nego_sig();
			_delay = PULSE_DURATION;
			goto exit_isr;
		}
		if (CHK_BIT (pkg_stage, NUM_SENDING)) {
			if(!(CHK_BIT (pkg_stage, POS_SIG_NUM)) && !(CHK_BIT (pkg_stage, ZER_SIG_NUM))) {
				number_send = numbers[current_num];
				SET_BIT(pkg_stage, POS_SIG_NUM);
			}
			if (0 != numbers[current_num]) {
				if (CHK_BIT (pkg_stage, POS_SIG_NUM)) {
					CLR_BIT(pkg_stage, POS_SIG_NUM);
					SET_BIT(pkg_stage, ZER_SIG_NUM);
					set_posi_sig();
					_delay = PULSE_DURATION;
					goto exit_isr;
				}
				if (CHK_BIT (pkg_stage, ZER_SIG_NUM)) {
					CLR_BIT(pkg_stage, ZER_SIG_NUM);
					SET_BIT(pkg_stage, POS_SIG_NUM);
					numbers[current_num]--;
					set_zero_sig();
					_delay = PULSE_DURATION;
					goto exit_isr;
				}
			} else {
				_delay = DELAY_AFTER_PULSE_TRAIN -  number_send * 2 * PULSE_DURATION;
				set_zero_sig();
				if (current_num < 5) {
					current_num++;
					pkg_stage = (1 << H_SYNC_PULSE);
				} else {
					current_num = 0;
					pkg_stage = 0;
				}
			}
		}
	}
exit_isr:
	return;
}
