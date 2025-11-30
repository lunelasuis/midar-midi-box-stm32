
#include "midi_device.h"
#include "usb_device.h"
#include "usbd_midi.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t eventBuffer[MIDI_EPIN_SIZE] = {0};
static uint8_t eventBufferIndex = 0;

void MIDI_addEventToBuffer(uint8_t message, uint8_t param1, uint8_t param2)
{
	eventBuffer[eventBufferIndex++] = (message >> 4);
	eventBuffer[eventBufferIndex++] = (message);
	eventBuffer[eventBufferIndex++] = (param1);
	eventBuffer[eventBufferIndex++] = (param2);

	if (eventBufferIndex == MIDI_EPIN_SIZE)
		MIDI_flush();

}

void MIDI_flush()
{
	if (eventBufferIndex == 0) return;
	while (USBD_MIDI_GetState(&hUsbDeviceFS) != MIDI_IDLE) {};
	USBD_MIDI_SendReport(&hUsbDeviceFS, eventBuffer, eventBufferIndex);
	eventBufferIndex = 0;
}
