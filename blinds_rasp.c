#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

struct gpiod_line *tempSensor;
struct gpiod_line *ldr;
struct gpiod_line *motorPin1;
struct gpiod_line *motorPin2;
struct gpiod_line *motorPin3;
struct gpiod_line *motorPin4;

/*
*	Set the speed in revs per minute
*/
unsigned long setSpeed(long whatSpeed){
	unsigned long stepDelay = 60L * 1000L *1000L / 32 / whatSpeed;
	return stepDelay;
}

/*
 * moves motor forward or backward
 */
void stepMotor(int thisStep){
	switch(thisStep){
		case 0: //1010
			gpiod_line_set_value(motorPin1, 1);
			gpiod_line_set_value(motorPin2, 0);
			gpiod_line_set_value(motorPin3, 1);
			gpiod_line_set_value(motorPin4, 0);
		break;
		case 1: //0110
			gpiod_line_set_value(motorPin1, 0);
			gpiod_line_set_value(motorPin2, 1);
			gpiod_line_set_value(motorPin3, 1);
			gpiod_line_set_value(motorPin4, 0);
		break;
		case 2: //0101
			gpiod_line_set_value(motorPin1, 0);
			gpiod_line_set_value(motorPin2, 1);
			gpiod_line_set_value(motorPin3, 0);
			gpiod_line_set_value(motorPin4, 1);
		break;
		case 3: //1001
			gpiod_line_set_value(motorPin1, 1);
			gpiod_line_set_value(motorPin2, 0);
			gpiod_line_set_value(motorPin3, 0);
			gpiod_line_set_value(motorPin4, 1);
		break;
	}
}

/*
 * Moves the motor stepsToMove steps. If the number is negative,
 * the motor moves in reverse  
 */
void step(int stepsToMove, int stepNum, unsigned long stepDelay){
	int stepsLeft = abs(stepsToMove);
	int direction;
	struct timespec t;
	
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	unsigned long lastStepTime = (t.tv_nsec)/1000;
	
	if(stepsToMove > 0) {direction = 1;}
	if(stepsToMove < 0)	{direction = 0;}
	
	//decrement the number of steps moving one step at a time
	while(stepsLeft > 0){
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
		unsigned long now = (t.tv_nsec)/1000;
		//move only if the delay has passed
		if(now - lastStepTime >= stepDelay){
			//time of when you stepped
			lastStepTime = now;
			if(direction == 1){
				stepNum++;
				if(stepNum == 32){
					stepNum = 0;
				}
			}else{
				if(stepNum == 0){
					stepNum = 32;
				}
				stepNum--;
			}
			stepsLeft--;
			stepMotor(stepNum % 4);
		}
	}
}

double getTemp(){
	//Read analog sensor input and store it	
	double temp = gpiod_line_get_value(tempSensor) / 1024;
	
	temp = temp * 5;			//multiply by 5V to get voltage
	temp = temp - 0.5;			//subtract the offset
	temp = temp * 100;			//Convert to degrees C
	temp = (temp * 1.8) + 32;	//Convert to Fahrenheit
	return temp;
}


int main(int argc, char **argv){
	//char *chipname = "gpiochip0";
	struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
	int light, pos = 0;
	double temp;
	
	//Open GPIO Chip
	//chip = gpiod_chip_open_by_name("gpiochip0");

	//Open GPIO Lines
	tempSensor = gpiod_chip_get_line(chip, 21);
	ldr = gpiod_chip_get_line(chip, 22);
	motorPin1 = gpiod_chip_get_line(chip, 13);
	motorPin2 = gpiod_chip_get_line(chip, 19);
	motorPin3 = gpiod_chip_get_line(chip, 26);
	motorPin4 = gpiod_chip_get_line(chip, 25);

	//Open GPIO lines for output
	gpiod_line_request_output(motorPin1, "example1", 0);
	gpiod_line_request_output(motorPin2, "example1", 0);
	gpiod_line_request_output(motorPin3, "example1", 0);
	gpiod_line_request_output(motorPin4, "example1", 0);
	

	//Open GPIO lines for input
	gpiod_line_request_input(tempSensor ,"example1");
	gpiod_line_request_input(ldr ,"example1");

	while(1){
		long speed = setSpeed(100);
		light = gpiod_line_get_value(ldr);
		printf("Light: %d\n", light);
		//if light is < 100 close blinds
		if(light < 100){
			//if pos > 0 then move back to 0 otherwise bllinds already closed
			if(pos == 8){
				step(-2048/8, 0, speed);
				pos = 0;
			}
			else if(pos == 4){
				step(-2048/4, 0, speed);
				pos = 0;
			}
		}
		//if light is > 100 but < 400 partial open
		if(light > 100 && light < 400){
			if(pos == 0){
				step(2048/8, 0, speed);
				pos = 8;
			}
			else if(pos == 4){
				step(-2048/8, 0, speed);
				pos = 8;
			}
		}
		//if light is > 400 open all the way
		if(light > 400){
			if(pos == 0){
				step(2048/4, 0, speed);
				pos = 4;
			}
			else if(pos == 8){
				step(2048/8, 0, speed);
				pos = 4;
			}
		}
		//if the temp is higher than 80F partially close blinds
		temp = getTemp();
		printf("Temperature: %f\n", temp);
		if(temp > 80.0){
			if(pos == 4){
				step(-2048/8, 0, speed);
				pos = 8;
			}
		}
		sleep(5);
	}

	gpiod_line_release(tempSensor);
  	gpiod_line_release(ldr);
  	gpiod_line_release(motorPin1);
  	gpiod_line_release(motorPin2);
	gpiod_line_release(motorPin3);
	gpiod_line_release(motorPin4);
  	gpiod_chip_close(chip);
	
	return 0;
}
