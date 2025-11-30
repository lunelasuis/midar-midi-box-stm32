#ifndef IOMATRIX_H
#define IOMATRIX_H

#define KEY_COUNT 49
//#define TOGGLE_COUNT 98 // 2 * KEY_COUNT

#define INPUT_PIN_COUNT 8
#define OUTPUT_PIN_COUNT 14

typedef struct IO_Pin
{
	GPIO_TypeDef * port;
	uint16_t pin;
} IO_Pin;

const IO_Pin inputPins[INPUT_PIN_COUNT]={
		{ GPIOB, GPIO_PIN_10 },
		{ GPIOC, GPIO_PIN_13 },
		{ GPIOB, GPIO_PIN_1 },
		{ GPIOB, GPIO_PIN_0 },
		{ GPIOA, GPIO_PIN_7 },
		{ GPIOA, GPIO_PIN_6 },
		{ GPIOA, GPIO_PIN_5 },
		{ GPIOA, GPIO_PIN_4 }
};

const IO_Pin outputPins[OUTPUT_PIN_COUNT]={
		{ GPIOA, GPIO_PIN_15 },
		{ GPIOB, GPIO_PIN_3 },
		{ GPIOB, GPIO_PIN_4 },
		{ GPIOB, GPIO_PIN_5 },
		{ GPIOB, GPIO_PIN_6 },
		{ GPIOB, GPIO_PIN_7 },
		{ GPIOB, GPIO_PIN_8 },
		{ GPIOB, GPIO_PIN_9 },
		{ GPIOA, GPIO_PIN_3 },
		{ GPIOB, GPIO_PIN_13 },
		{ GPIOB, GPIO_PIN_14 },
		{ GPIOA, GPIO_PIN_0 },
		{ GPIOC, GPIO_PIN_14 },
		{ GPIOC, GPIO_PIN_15 }
};

const IO_Pin sustainToggle = { GPIOA, GPIO_PIN_9 };

const IO_Pin octaveUpButton = { GPIOB, GPIO_PIN_15 };
const IO_Pin octaveDownButton = { GPIOA, GPIO_PIN_8 };

#endif
