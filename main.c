#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>

#define BTNSKIP 16

volatile enum {
	M_AUTO = 0, M_OFF, M_QUART, M_HALF, M_ON, M_STROBO, M_INVAL
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
	OCR1A = 0x1ff;
	TCCR1A = 0;
	TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
	TIMSK = _BV(OCIE1A);

	mode = M_AUTO;

	sei();

	while (1) {
		MCUCR |= _BV(SE);
		asm("sleep");
	}

	return 0;
}

ISR(TIMER1_COMPA_vect)
{
	static char done = 0;
	static unsigned char skip = 0;
	cli();
	asm("wdr");

	if (skip)
		skip--;
	else if (~PIND & _BV(PD6)) {
		skip = BTNSKIP;
		mode = (mode + 1) % M_INVAL;
		TCCR0A = 0;
		TCCR0B = 0;
		CLKPR = _BV(CLKPCE);
		CLKPR = 0;
	}

	if (mode == M_AUTO) {
		if (ACSR & _BV(ACO))
			PORTD &= ~_BV(PD5);
		else
			PORTD |= _BV(PD5);
		return;
	}
	if (mode == M_OFF) {
		if (skip == BTNSKIP)
			PORTD &= ~_BV(PD5);
		return;
	}
	if (mode == M_QUART) {
		if (skip == BTNSKIP) {
			OCR0B = 64;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
		return;
	}
	if (mode == M_HALF) {
		if (skip == BTNSKIP) {
			OCR0B = 128;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
		return;
	}
	if (mode == M_ON) {
		if (skip == BTNSKIP)
			PORTD |= _BV(PD5);
		return;
	}
	if (mode == M_STROBO) {
		if (skip == BTNSKIP) {
			CLKPR = _BV(CLKPCE);
			CLKPR = _BV(CLKPS0);
			OCR0B = 128;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS02) | _BV(CS00);
		}
		return;
	}


/*

	if (PIND & _BV(PD4)) {
		if (done)
			return;
		if (!OCR0B) {
			OCR1A = 0x04;
			OCR0B = 0x01;
			TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
			TCCR0B = _BV(CS00);
		}
		else if (OCR0B < 0xfe) {
			OCR0B++;
		}
		else {
			done = 1;
			OCR1A = 0xff;
			TCCR0A = 0;
			TCCR0B = 0;
			PORTD |= _BV(PD5);
		}
	}
	else {
		done = 0;
		OCR0B = 0;
		TCCR0A = 0;
		TCCR0B = 0;
		PORTD &= ~_BV(PD5);
	}
*/
}
