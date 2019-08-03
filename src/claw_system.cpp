#include <Arduino.h>

#include "claw_system.h"

// //crane pushbutton booleans
// volatile bool zIsHome = 0;
// volatile bool yIsHome = 0;
// volatile bool zIsExtended = 0;
// volatile bool yIsExtended = 0;
// volatile bool clawPBPressed = 0;
// volatile bool clawBasePBPressed = 0;

//timer ISR variables
volatile byte motTimeControl; //a byte consisting of flags for the timerISR to use in processing
volatile int stepsLeft;       //the number of steps left for the stepper motor to move
volatile int zTimeInc;        //the number of z time increments that have passed since the last variable reset
volatile int yTimeInc;        //the number of y time increments passed since the last variable reset
volatile int clawTimeInc;
volatile int timePerInc;     //the software defined time interval of the timer interrupt
const int timePerStep = 0;   //the user defined time per step, gives the speed of the stepper motor
const int totalYTime = 0;    //the user defined time per step, gives the speed of the stepper motor
const int totalClawTime = 0; //the user defined time per step, gives the speed of the stepper motor

int globalcount = 0;
//global status variables
volatile double yPos = 0.0;
volatile int yStatus = NOT_MOVING;
volatile int clawStatus = NOT_MOVING;
volatile bool clawIsOpen = false;
volatile bool clawRecentAction = CLOSED;    //the most recent claw action, held as a boolean
volatile bool crystalInPouch = false;

// void zHomeISR()
// {
//     //Serial.println("zhome");
//     zIsHome = true;
// }

// void zFullExtISR()
// {
//     //Serial.println("zFullExtISR");
//     zIsExtended = true;
// }

void yHomeISR()
{
    //Serial.println("yHomeISR");
    if (yStatus == MOVING_BK)
    {
        pwm_stop(Y_SERVO_PWM_NAME);
        yStatus = NOT_MOVING;
    }
    //Serial.println("Y=0 limit reached");
    yPos = 0;
    //yIsHome = true;
    //yIsExtended = false;
}

void yFullExtISR()
{
    //Serial.println("yFullExtISR");
    if (yStatus == MOVING_FWD)
    {
        pwm_stop(Y_SERVO_PWM_NAME);
        yStatus = NOT_MOVING;
    }
    //Serial.println("Y max limit reached");
    yPos = 300;
    //yIsExtended = true;
    //yIsHome = false;
}

// void clawPBISR()
// {
//     //Serial.println("clawPBISR");
//     clawPBPressed = true;
// }

// void clawFloorPBISR()
// {
//     //Serial.println("clawFloorPBISR");
//     clawBasePBPressed = true;
// }

void openClaw(int duration)
{
    
    //Serial.println("Opening claw");
    pwm_start(
        CLAW_SERVO_PWM_NAME,
        PWM_CLOCK_FREQ,
        FWD_PERIOD,
        FWD_ON_PERIOD,
        1);
    clawStatus = MOVING_FWD;
    //Serial.println("PWM to open claw started");

    delay(duration);

    pwm_stop(CLAW_SERVO_PWM_NAME);

    clawStatus = NOT_MOVING;
    clawIsOpen = true;
    clawRecentAction = OPENED;
}

//duration is the time the servo is on for
void closeClaw(int duration)
{

    //Serial.println("Closing claw");
    pwm_start(
        CLAW_SERVO_PWM_NAME,
        PWM_CLOCK_FREQ,
        BK_PERIOD,
        BK_ON_PERIOD,
        1);
    // Serial.println("PWM to close claw started");
    clawStatus = MOVING_BK;
    // Serial.println("PWM to open claw started");

    delay(duration);

    pwm_stop(CLAW_SERVO_PWM_NAME);

    clawStatus = NOT_MOVING;
    clawIsOpen = false;
    clawRecentAction = CLOSED;
}

void homeY(bool retract)
{
    unsigned long timeoutStart = millis();
    unsigned long timeoutNow = millis();

    // Serial.println("Starting Y home");
    // yIsHome = digitalRead(YHOME);
    // yIsExtended = digitalRead(YFULLEXT);
    if (!digitalRead(YHOME) && retract)
    {
        // Serial.println("Y not home");
        pwm_start(
            Y_SERVO_PWM_NAME,
            PWM_CLOCK_FREQ,
            BK_PERIOD,
            BK_ON_PERIOD,
            1);
        yStatus = MOVING_BK;
        // Serial.println("Y homing started");
        while (!digitalRead(YHOME) || timeoutNow - timeoutStart >= Y_HOME_TIMEOUT)
        {
            timeoutNow = millis();
        }
        timeoutNow = millis();
        // Serial.print("ms for home operation: ");
        // Serial.println(timeoutNow - timeoutStart);
    }
    else if (!digitalRead(YFULLEXT) && !retract)
    {
        // Serial.println("Y not extended");
        pwm_start(
            Y_SERVO_PWM_NAME,
            PWM_CLOCK_FREQ,
            FWD_PERIOD,
            FWD_ON_PERIOD,
            1);
        yStatus = MOVING_FWD;
        // Serial.println("Y extension started");
        while (!digitalRead(YFULLEXT) || timeoutNow - timeoutStart >= Y_HOME_TIMEOUT)
        {
            timeoutNow = millis();
        }
        timeoutNow = millis();
        // Serial.print("ms for extend operation: ");
        // Serial.println(timeoutNow - timeoutStart);
    }
    else
    {
        // Serial.println("Invalid conditions for this home operation");
        pwm_stop(Y_SERVO_PWM_NAME);
    }
}

void moveY(double dist)
{
    double timeToRun = 0;
    if (dist < 0)
    {
        timeToRun = abs(dist) * (60000 / (134.05 * 62.832));
        pwm_start(
            Y_SERVO_PWM_NAME,
            PWM_CLOCK_FREQ,
            BK_PERIOD,
            BK_ON_PERIOD,
            1);
        yStatus = MOVING_BK;
    }
    else if (dist > 0)
    {
        timeToRun = abs(dist) * (60000 / (129.05 * 62.832));
        pwm_start(
            Y_SERVO_PWM_NAME,
            PWM_CLOCK_FREQ,
            FWD_PERIOD,
            FWD_ON_PERIOD,
            1);
        yStatus = MOVING_FWD;
    }
    //   Serial.print("Starting PWM for time = ");
    //   Serial.println(timeToRun);
    delay(timeToRun);
    pwm_stop(Y_SERVO_PWM_NAME);
    yStatus = NOT_MOVING;
    yPos += dist;
}

// void moveYUntilClawPressed()
// {

//     clawPBPressed = false;

//     pwm_start(
//                 Y_SERVO_PWM_NAME,
//                 PWM_CLOCK_FREQ,
//                 FWD_PERIOD,
//                 FWD_ON_PERIOD,
//                 1);
//     yStatus = MOVING_FWD;

//     int run_time = 0;

//     while(!clawPBPressed) {  
//         run_time++;
//         delay(1);
//     }

//     clawPBPressed = false;
//     pwm_stop(Y_SERVO_PWM_NAME);
//     yStatus = NOT_MOVING;
//     yPos += run_time * (129.05 * 62.832) / 60000;
// }

void findTopOfPillar(int delay)
{
    //clawBasePBPressed = false;
    if (!digitalRead(CLAWFLOORPB))
        return;
    changeStepperDir(DOWN);
    while (digitalRead(CLAWFLOORPB))
    {
        stepperPulse(delay);
    }
    //clawBasePBPressed = false;
}

//returns the number of steps moved to get to either switch
int moveZToExtreme(bool home, int delay)
{
    //set the home flags to false to allow for the positive edge of the interrupts to cause a rising flag
    // zIsHome = 0;
    // zIsExtended = 0;

    int count = 0;

    if (home == true)
    {
        if (digitalRead(ZHOME))
            return count; //if it is sensed that z is home, quit this protocal since a rising edge interrupt can not occur
        digitalWrite(STEPPERDIR, DOWN);
        while (!digitalRead(ZHOME))
        {
            count++;
            stepperPulse(delay);
        }
        //zIsHome = false;
    }
    else
    {
        digitalWrite(STEPPERDIR, UP);
        while (!digitalRead(ZFULLEXT))
        {
            count++;
            stepperPulse(delay);
        }
        //zIsExtended = false;
    }
    return count;
}

void changeStepperDir(bool dir)
{
    digitalWrite(STEPPERDIR, dir);
}

void enableStepper()
{
    digitalWrite(STEPPERCLK, LOW);
}

void disableStepper()
{
    digitalWrite(STEPPERCLK, HIGH);
}

/* 
StepperPulse sends a pwm signal to the CLK pin of a stepper motor driver
 */
void stepperPulse(int delay)
{
    digitalWrite(STEPPERCLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(STEPPERCLK, LOW);
    delayMicroseconds(delay);
}

//moves the z axis for "steps" steps
int moveZSteps(int steps, bool dir, int delay)
{
    if (dir == UP)
    {
        changeStepperDir(UP);
        for (int i = 0; i < steps; i++)
        {
            if (digitalRead(ZFULLEXT))
            {
                //zIsExtended = false;
                return i;
            }
            stepperPulse(delay);
        }
        return steps;
    }
    else
    {
        changeStepperDir(DOWN);
        for (int i = 0; i < steps; i++)
        {
            if (digitalRead(ZHOME))
            {
                //zIsHome = false;
                return i;
            }
            stepperPulse(delay);
        }
        return steps;
    }
}

//moves z for milimeters in the specified direction, returns steps taken
int moveZDist(bool dir, int mm, int delay) {
    int steps = mmToSteps(mm);

    return moveZSteps(steps, dir, delay);
}

int mmToSteps(int mm)
{
    return (float) mm * (float) 820 / (float) 299; //truncates the float, this precision is not necessary. ie 2.3 steps.
}


//returns true if a crystal was grabbed
//-1 for fully extending, 0 for tallest, 1 for medium, 2 for smallest
bool grabCrystal(int pillarType)
{
    digitalWrite(STEPPERENABLE, LOW);
    switch(pillarType) {
        case -1: moveZToExtreme(EXTEND, 1800);
        break;

        case 0: moveZDist(UP, 270, 1900);
        break;

        case 1: moveZDist(UP, 190, 1900);
        break;

        case 2: moveZDist(UP, 130, 1900);
        break;
    }

    moveY(62);
    openClaw(1300);
    findTopOfPillar(1500);
    moveY(15);
    closeClaw(3000);
    moveZDist(UP, 50, 2500);
    homeY(true);
    
    // //if you dont want to put the crystal in the pouch or if there already is one, just drop the stepper down
    // if (!putInPouch || crystalInPouch) {   
    //     digitalWrite(STEPPERENABLE, HIGH);
    // //if there is no crystal in the pouch and you want the crystal in it, move z home and let go
    // } else {
    //     moveZToExtreme(HOME,1800);
    //     openClaw(500);
    // }

    disableStepper();
    
    if (digitalRead(CLAWPB)) {
        //clawPBPressed = false;
        return true;
    } else {
        return false;
    }
}

// gauntlet pos: -1 for full extend, 0 for farthest two from robot, 1 for middle two, 2 for closest two 
// inClaw is boolean for if the crystal is already in the claw or in the pouch (true iff in claw)
void depositCrystal(int gauntletPos, bool inClaw) 
{
    enableStepper();

    if (inClaw) {
        closeClaw(500); //make sure the claw is holding the crystal
    } else {   
        //if the claw was most recently opened it is still open, just close it
        if (clawRecentAction == OPENED) {
            moveZToExtreme(HOME,1700);
            closeClaw(2000);
        //if the claw is closed move up, open, and then grab the crystal
        } else {
            moveZDist(UP,50,1800);
            openClaw(700);
            moveZToExtreme(HOME,1700);
            closeClaw(2000);
        }
    }

    //move z to the right position
    switch(gauntletPos) {
        case -1: moveZToExtreme(EXTEND, 2000);
        break;
        case 0: moveZDist(UP, 150, 2000);
        break;
        case 1: moveZDist(UP, 110, 2000);
        break;
        case 2:moveZDist(UP, 80, 2000);
        break;
    }

    //for now no special cases
    switch(gauntletPos) {
        case -1: homeY(EXTEND);
        break;
        case 0: homeY(EXTEND);
        break;
        case 1: homeY(EXTEND);
        break;
        case 2: homeY(EXTEND);
        break;
    }

    disableStepper();   //drop the claw onto the gauntlet
    delay(500);
    openClaw(500);
    enableStepper();
    moveZDist(UP, 70, 1800);
    homeY(HOME);

    disableStepper();   //leaves the claw open currently
}

