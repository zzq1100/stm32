#include "stm32f10x.h"                  // Device header
#include "Buzzer.h"
#include "LightSensor.h"
#include "Delay.h"

int main()
{
	Buzzer_Init();
	LightSensor_Init();
	
	while (1)
	{
		if (LightSensor_Get() == 1)
			Buzzer_OFF();
		else
			Buzzer_OFF();
	}
}
