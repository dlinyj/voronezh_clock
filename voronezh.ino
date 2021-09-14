#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <inttypes.h>
#include <DS1307RTC.h>
#include <Wire.h>

#define POS_SIG	2
#define NEG_SIG	3

tmElements_t m_time = {0};
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);// RS E

volatile boolean blink = true;
volatile uint8_t numbers[6] 	={0};

// keypad keys definitions
enum KP
{
  KEY_RIGHT = 0,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_SELECT,
  KEY_NUMKEYS, // mark the end of key definitions
  KEY_NONE     // definition of none of the keys pressed
};

// Finite Machine States
enum FMS
{
  RUN = 0,
  SETHOUR,
  SETMINUTE,
  SETDAY,
  SETMONTH,
  SETYEAR
} ClockState;

// Finite Machine State Transitions Table.
// Defines the flow of the application modes from one to another.
enum FMS StateMachine[] =
{
   /* RUN        -> */ SETHOUR,
   /* SETHOUR    -> */ SETMINUTE,
   /* SETMINUTE  -> */ SETDAY,
   /* SETDAY     -> */ SETMONTH,
   /* SETMONTH   -> */ SETYEAR,
   /* SETYEAR    -> */ RUN
};

enum KP key = KEY_NONE;


inline void set_zero_sig() __attribute__((always_inline));
inline void set_posi_sig() __attribute__((always_inline));
inline void set_nego_sig() __attribute__((always_inline));

void setupTimer1() {
	noInterrupts();
	// Clear registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	// 12003.000750187546 Hz (16000000/((1332+1)*1))
	OCR1A = 1332;
	// CTC
	TCCR1B |= (1 << WGM12);
	// Prescaler 1
	TCCR1B |= (1 << CS10);
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

void read_key () {
	key = KEY_NONE;
	int keyAnalog =  analogRead(A0);
	if ((keyAnalog < 800) && (keyAnalog > 600)) { //select = reset
		key = KEY_SELECT;
		goto exit;
	}
	if (keyAnalog < 100) { //right
		key = KEY_RIGHT;
		goto exit;
	}
	if ((keyAnalog < 600) && (keyAnalog > 400)) { //left
		key = KEY_LEFT;
		goto exit;
	}
	if ((keyAnalog < 200) && (keyAnalog > 130)) { //up
		key = KEY_UP;
		goto exit;
	}
	if ((keyAnalog < 400) && (keyAnalog > 200)) { //down
		key = KEY_DOWN;
		goto exit;
	}

exit:
	return;
}

boolean test_leap_year(uint8_t year)  {
	uint16_t current_year = tmYearToCalendar(year);
	if ((((current_year%400)==0) || ((current_year%4)==0)) && !((current_year%100)==0)) {
		return true;
	} else {
		return false;
	}
}

void load_time () {
	noInterrupts();
	numbers[0]	= m_time.Hour / 10;
	numbers[1]	= m_time.Hour % 10;
	numbers[2]	= m_time.Minute / 10;
	numbers[3]	= m_time.Minute % 10;
	numbers[4]	= m_time.Second /10;
	numbers[5]	= m_time.Second % 10;
	interrupts();
}

uint8_t  settime () {
	int set = 0;
	const unsigned int month_days [] =
		{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	RTC.read(m_time); //read time from DS3231
	load_time ();
	//delay(200)
	static unsigned long old_time = 0;
	static unsigned long new_time;
	static uint8_t blinker_counter = 0;
	if (3 == blinker_counter) {
		blinker_counter = 0;
		blink = ((blink) ? false : true);
	}

	new_time = millis();
	if ((new_time - old_time) >= 200) {
		blinker_counter++;
		old_time = new_time;

		read_key();
		if (key == KEY_SELECT) {
			ClockState = StateMachine[ClockState]; // switch mode
		}
		if (key == KEY_LEFT) {
			ClockState = RUN; // switch mode
		}
		if (ClockState != RUN) {
			if (key == KEY_RIGHT) {
				set++;
				m_time.Second = 0;
			}

			if (key == KEY_UP) {
				switch (ClockState) {
					case SETHOUR:
						set++;
						m_time.Hour++;
						if (m_time.Hour > 23) m_time.Hour = 0;
						break;	
					case SETMINUTE:
						set++;
						m_time.Minute++;
						if (m_time.Minute > 59) m_time.Minute = 0;
						break;
					case SETDAY:
						set++;
							if ((m_time.Month != 2 && m_time.Day < month_days[m_time.Month])
								||
								(m_time.Month == 2 && m_time.Day < 28)
								||
								(m_time.Month == 2 && m_time.Day == 28 &&
								test_leap_year(m_time.Year))) {
									m_time.Day++;
							}
						break;
					case SETMONTH:
						set++;
						m_time.Month++;
						if (m_time.Month > 12) {
							m_time.Month = 1;
						}
						break;
					case SETYEAR:
						set++;
						m_time.Year++;
						break;
				}
			}
			if (key == KEY_DOWN) {
				switch (ClockState) {
					case SETHOUR:
						set++;
						if (m_time.Hour == 0) {
							m_time.Hour = 23;
						} else {
							m_time.Hour--;
						}
						break;
					case SETMINUTE:
						set++;
						if (m_time.Minute == 0) {
							m_time.Minute = 59;
						} else {
							m_time.Minute--;
						}
						break;
					case SETDAY:
						set++;
						if (m_time.Day > 1) {
							m_time.Day--;
						}
						break;
					case SETMONTH:
						set++;
						if (m_time.Month > 1) {
							m_time.Month--;
						} else {
							m_time.Month = 12;
						}

						break;
					case SETYEAR:
						set++;
						if (m_time.Year > 0) {
							m_time.Year--;
						}
						break;
				}
			}

			if (set) {
				if (m_time.Month != 2 && m_time.Day > month_days[m_time.Month]) {
					m_time.Day = month_days[m_time.Month];
				}
				if (m_time.Month == 2) {
					if (test_leap_year(m_time.Year) && m_time.Day >= 29) {
						m_time.Day = 29;
					} else {
						if (m_time.Day >= 28) {
							m_time.Day = 28;
						}
					}
				}
				load_time ();
				return RTC.write(m_time);
			}
		}
	} else {
		return 0;
	}
}

void print_the_time () {
	if (!blink && ClockState == SETHOUR) {
		lcd.setCursor(0, 0);
		lcd.print("  ");
	} else {
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
	if (!blink && ClockState == SETMINUTE) {
		lcd.setCursor(3, 0);
		lcd.print("  ");
	} else {
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
	}

	if (m_time.Second < 10) { //если секунд меньше 10
		lcd.setCursor(6, 0); // курсор в 6 позицию 0 строки
		lcd.print(0); //печатаем 0
		lcd.setCursor(7, 0); //курсор в 7 позицию 0 строки
		lcd.print(m_time.Second); //печатаем значение переменной s
	}	else {
		lcd.setCursor(6, 0); //иначе курсор в 6 позицию 0 строки
		lcd.print(m_time.Second); // печатаем значение переменной s
	}
	
	if (!blink && ClockState == RUN) {
		lcd.setCursor(5, 0); // курсор в 5 позицию 0 строки
		lcd.print(" "); //  печатаем разделитель между секундами и минутами
		lcd.setCursor(2, 0); // курсор в 2 позицию 0 строки
		lcd.print(" "); //  печатаем разделитель между минутами и часами
	} else {
		lcd.setCursor(5, 0); // курсор в 5 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между секундами и минутами
		lcd.setCursor(2, 0); // курсор в 2 позицию 0 строки
		lcd.print(":"); //  печатаем разделитель между минутами и часами
	}

//Печатаем дату

	if (!blink && ClockState == SETDAY) {
			lcd.setCursor(0, 1);
			lcd.print("  ");
		} else {
		if (m_time.Day < 10) {
			lcd.setCursor(0, 1);
			lcd.print(0);
			lcd.setCursor(1, 1);
			lcd.print(m_time.Day);
		} else {
			lcd.setCursor(0, 1);
			lcd.print(m_time.Day);
		}
	} 
	lcd.setCursor(2, 1); 
	lcd.print("."); 
	if (!blink && ClockState == SETMONTH) {
			lcd.setCursor(3, 1);
			lcd.print("  ");
	} else {	
		if (m_time.Month < 10) {
				lcd.setCursor(3, 1);
				lcd.print(0);
				lcd.setCursor(4, 1);
				lcd.print(m_time.Month);
		} else {
			lcd.setCursor(3, 1);
			lcd.print(m_time.Month);
		}
	}
	lcd.setCursor(5, 1);
	lcd.print("."); 
	if (!blink && ClockState == SETYEAR) {
			lcd.setCursor(6, 1); 
			lcd.print("    ");
	} else {
		lcd.setCursor(6, 1); 
		lcd.print(tmYearToCalendar(m_time.Year)); //печатаем 0
	}
}

void loop() {
	settime();
	print_the_time ();
}

void set_zero_sig() {
	PORTD |= (1 << POS_SIG) | (1 << NEG_SIG);
}

void set_posi_sig() {
	PORTD = PORTD &~ (1 << POS_SIG) | (1 << NEG_SIG);
}

void set_nego_sig() {
	PORTD = PORTD &~ (1 << NEG_SIG) | (1 << POS_SIG);
}

ISR(TIMER1_COMPA_vect) {
	static uint8_t frame_counter = 0;
	static uint8_t cur_pos_num = 0;
	if (frame_counter < 20) {
		if (frame_counter & 1) {
			if (numbers[cur_pos_num] == (9 - (frame_counter >> 1))) {
				set_nego_sig();
			} else {
				set_zero_sig();
			}
		} else {
			set_posi_sig();
		}
	}
	if (frame_counter++ == 39) {
		frame_counter = 0;
		if (cur_pos_num < 5) {
			cur_pos_num++;
		} else {
			set_nego_sig(); 
			cur_pos_num = 0;
		}
	} else {
		if (frame_counter > 20) {
			set_zero_sig();
		}
	}
}
