#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>

#define BTNSKIP 8

volatile enum {
	M_MAN = 0, M_QUART, M_HALF, M_AUTO, M_STROBO, M_INVAL
} mode;

int main (void)
{
	CLKPR = _BV(CLKPCE);
	CLKPR = 0;

	MCUSR = 0;
	WDTCSR = _BV(WDE) | _BV(WDP3) | _BV(WDP0);

	DDRD = _BV(DDD5);
	PORTD = _BV(PD6);

	DIDR = _BV(AIN1D) | _BV(AIN0D);

	OCR0B = 0x00;
	OCR1A = 0x300;
	TCCR1A = 0;
	TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
	TIMSK = _BV(OCIE1A);

	mode = M_MAN;

	sei();

	while (1) {
		MCUCR |= _BV(SE);
		asm("sleep");
	}

	return 0;
}

ISR(TIMER1_COMPA_vect)
{
	static unsigned char skip = 0, repeat = 0, mode_changed = 0;
	static unsigned char now = 0, target = 0xff;
	cli();
	asm("wdr");

	if (skip)
		skip--;
	else if (~PIND & _BV(PD6)) {
		skip = BTNSKIP;
		repeat++;
	}
	else if (repeat == 1) {
		target = ~target;
		repeat = 0;
	}
	if (repeat == 2) {
		mode = (mode + 1) % M_INVAL;
		mode_changed = 1;
		repeat = 0;
	}

	if ((mode == M_MAN) && (now != target)) {
		if ((now == 0) || (now == 255)) {
			OCR0B = now;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
		if ((now == 1) && (target == 0)) {
			TCCR0A = TCCR0B = 0;
			now = target;
			PORTD &= ~_BV(PD5);
		}
		else if ((now == 254) && (target == 255)) {
			TCCR0A = TCCR0B = 0;
			now = target;
			PORTD |= _BV(PD5);
		}
		else if (now > target)
			OCR0B = --now;
		else if (now < target)
			OCR0B = ++now;
	}


	if (mode == M_AUTO) {
		if (mode_changed) {
			CLKPR = _BV(CLKPCE);
			CLKPR = 0;
			now = 0;
		}
		if (ACSR & _BV(ACO))
			target = 0;
		else
			target = 255;
	}
	else if (mode == M_MAN) {
		if (mode_changed) {
			CLKPR = _BV(CLKPCE);
			CLKPR = 0;
			now = 0;
			target = 0;
		}
	}
	else if (mode == M_QUART) {
		if (mode_changed) {
			OCR0B = 64;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
	}
	else if (mode == M_HALF) {
		if (mode_changed) {
			OCR0B = 128;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
	}
	else if (mode == M_STROBO) {
		if (mode_changed) {
			CLKPR = _BV(CLKPCE);
			CLKPR = _BV(CLKPS0);
			OCR0B = 128;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS02) | _BV(CS00);
		}
	}
	mode_changed = 0;
}
