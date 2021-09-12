#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <inttypes.h>
#include <DS1307RTC.h>
#include <Wire.h>

#define POS_SIG	2
#define NEG_SIG	3

tmElements_t m_time = {0};
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);// RS E

boolean blink = true;

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

	ClockState = RUN;
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

uint8_t  settime () {
	tmElements_t l_time = {0};
	int set = 0;
	const unsigned int month_days [] =
		{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	RTC.read(l_time); //read time from DS3231
	noInterrupts();
	m_time = l_time; //send time to output function
	interrupts();
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
				l_time.Second = 0;
			}

			if (key == KEY_UP) {
				switch (ClockState) {
					case SETHOUR:
						set++;
						l_time.Hour++;
						if (l_time.Hour > 23) l_time.Hour = 0;
						break;	
					case SETMINUTE:
						set++;
						l_time.Minute++;
						if (l_time.Minute > 59) l_time.Minute = 0;
						break;
					case SETDAY:
						set++;
							if ((l_time.Month != 2 && l_time.Day < month_days[l_time.Month])
								||
								(l_time.Month == 2 && l_time.Day < 28)
								||
								(l_time.Month == 2 && l_time.Day == 28 &&
								(((l_time.Year%400)==0) || ((l_time.Year%4)==0)) && ! ((l_time.Year%100)==0))
								) {
									l_time.Day++;
							}
						break;
					case SETMONTH:
						set++;
						l_time.Month++;
						if (l_time.Month > 12) {
							l_time.Month = 1;
						}
						break;
					case SETYEAR:
						set++;
						l_time.Year++;
						break;
				}
			}
			if (key == KEY_DOWN) {
				switch (ClockState) {
					case SETHOUR:
						set++;
						if (l_time.Hour == 0) {
							l_time.Hour = 23;
						} else {
							l_time.Hour--;
						}
						break;
					case SETMINUTE:
						set++;
						if (l_time.Minute == 0) {
							l_time.Minute = 59;
						} else {
							l_time.Minute--;
						}
						break;
					case SETDAY:
						set++;
						if (l_time.Day > 1) {
							l_time.Day--;
						}
						break;
					case SETMONTH:
						set++;
						if (l_time.Month > 1) {
							l_time.Month--;
						} else {
							l_time.Month = 12;
						}

						break;
					case SETYEAR:
						set++;
						if (l_time.Year > 0) {
							l_time.Year--;
						}
						break;
				}
			}


			if (set) {
				if (l_time.Month != 2 && l_time.Day > month_days[l_time.Month]) {
					l_time.Day = month_days[l_time.Month];
				}
				if (l_time.Month == 2) {
					if ((((l_time.Year%400)==0) || ((l_time.Year%4)==0)) && ! ((l_time.Year%100)==0)
						&& l_time.Day > 29) {
						l_time.Day = 29;
					} else {
						if (l_time.Day > 28) {
							l_time.Day = 28;
						}
					}
				}
				noInterrupts();
				m_time = l_time; //send time to timer function
				interrupts();
				return RTC.write(l_time);
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

#define PULSE_DURATION	100
#define TIMER_PERIOD	50
#define DELAY_AFTER_PULSE_TRAIN	1600

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
	static int8_t numbers[6] 	={0};
	static uint8_t number_send	= 0;
	static uint8_t current_num	= 0;
	static uint8_t pkg_stage	= 0;
	static uint16_t _delay	 	= 0;
	static uint8_t tenner		= 10;
#ifdef DEBUG
	static int exit_stage = 0;
#endif
	if (_delay >= TIMER_PERIOD) {
#ifdef DEBUG
		print_iterator ();
#endif
		_delay-=TIMER_PERIOD;
		goto exit_isr;
	} else {
		_delay = 0;
#ifdef DEBUG
		if (exit_stage) exit(1);
#endif
	}
	if (0 == pkg_stage) {
		//SET_BIT(pkg_stage, H_SYNC_PULSE); //h-sync pulse stage
		SET_BIT(pkg_stage, NUM_SENDING);
		numbers[0]	= 10 - m_time.Hour / 10;
		numbers[1]	= 10 - m_time.Hour % 10;
		numbers[2]	= 10 - m_time.Minute / 10;
		numbers[3]	= 10 - m_time.Minute % 10;
		numbers[4]	= 10 - m_time.Second /10;
		numbers[5]	= 10 - m_time.Second % 10;
		//first pulse
		set_nego_sig();
		_delay 		= PULSE_DURATION;
		goto exit_isr;
	} else {
		set_zero_sig();
/*
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
*/
		if (CHK_BIT (pkg_stage, NUM_SENDING)) {
			if(!(CHK_BIT (pkg_stage, POS_SIG_NUM)) && !(CHK_BIT (pkg_stage, ZER_SIG_NUM))) {
				number_send = numbers[current_num];
				tenner = 10;
				SET_BIT(pkg_stage, POS_SIG_NUM);
			}
			if (0 != tenner) {
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
					tenner--;
					numbers[current_num]--;
					if (numbers[current_num] == 0) {
						set_nego_sig();
					} else {
						set_zero_sig();
					}
					_delay = PULSE_DURATION;
					goto exit_isr;
				}
			} else {
				_delay = DELAY_AFTER_PULSE_TRAIN;
				set_zero_sig();
				if (current_num < 5) {
					current_num++;
					tenner = 10;
					//pkg_stage = (1 << H_SYNC_PULSE);
				} else {
					current_num = 0;
					pkg_stage = 0;
#ifdef DEBUG
					exit_stage = 1;
#endif
				}
			}
		}
	}
exit_isr:
	return;
}
