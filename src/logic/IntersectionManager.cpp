#include <Arduino.h>

#include "sensors/MainTapeSensor.hpp"
#include "actuators/DriveSystem.hpp"
#include "logic/IntersectionManager.hpp"

#include "claw_system.h"

#define TURN_COUNTER_MAX 100
#define DELAY_TIME 200

unsigned long last_intersection_time = millis();

unsigned long gauntlet_timer;

// Constructor
IntersectionManager::IntersectionManager(MainTapeSensor* tape_sensor,
    DriveSystem* drive_system) {
        
    // Modules
    this->tape_sensor = tape_sensor;
    this->drive_system = drive_system;

    // State
    this->intersection_count = 2;

    this->gauntlet_state = 0;

}

void IntersectionManager::wiggle(int numOfWiggles, int wiggleHalfPeriod) {
    for (int i = 0; i < numOfWiggles; i++) {
        this->drive_system->update(0.93, -0.1);
        this->drive_system->actuate();
        delay(wiggleHalfPeriod);
        this->drive_system->update(-0.1, 0.93);
        this->drive_system->actuate();
        delay(wiggleHalfPeriod);
    }
}

void IntersectionManager::update() {

    // temp: handle gauntlet
    // TODO: manage intersection increments
    if (this->intersection_count == 7) {
        this->handle_gauntlet();
    } 

    // TEMP: for now
    else if (this->at_t_intersection()||this->at_y_intersection()) {

        unsigned long new_time = millis();
        //Debounce in case intersection triggered by accident due to oscilation 
        if(new_time - last_intersection_time >= DELAY_TIME) {
            this->handle_intersection();
            this->intersection_count++;
            Serial.println("AT INT");
            last_intersection_time = new_time;
        }
    }
}
//Y intersection defined as at least 2 subsequent black with at least one white followed by at least 2 subsequent black
bool IntersectionManager::at_y_intersection() {
    
    vector<bool> qrds_status = this->tape_sensor->get_qrds_status();

    // duplicate end elements for correct detection
    qrds_status.insert(qrds_status.begin(), *qrds_status.begin());
    vector<bool>::iterator it = qrds_status.end();
    advance(it, -1);
    qrds_status.push_back(*it);

    bool cond = false;

    it = qrds_status.begin();

    // Search for two blacks in a row
    int count = 0;
    for (; it != qrds_status.end(); it++) {
        if (*it) {
            count++;
            if (count == 1) {
                break;
            }
        }
    }

    // Skip whites + ensure at least one white
    count = 0;
    for (; it != qrds_status.end(); it++) {
        if(!*it) {
            count++;
        }
        else if (count >= 1) {
            break;
        }
    }
    // Search for two blacks in a row
    count = 0;
    for (; it != qrds_status.end(); it++) {
        if (*it) {
            count++;
            if (count == 1) {
                cond = true;
                break;
            }
        }
    }

    it = qrds_status.begin();
    bool val1 = *it;
    advance(it, 2);
    bool val2 = *it;

    it = qrds_status.end();
    bool val3 = *it;
    advance(it, -2);
    bool val4 = *it;

    bool cond2 = (val1 && val2) || (val3 && val4);

    return cond || cond2;

}
// T intersction required atleast 4 subsequent Black in a row
bool IntersectionManager::at_t_intersection() {
    
    vector<bool> qrds_status = this->tape_sensor->get_qrds_status();

    qrds_status.insert(qrds_status.begin(), *qrds_status.begin());
    vector<bool>::iterator it = qrds_status.end();
    advance(it, -1);
    qrds_status.push_back(*it);

    it = qrds_status.begin();

    bool cond = false;

    // Skip whites
    for (; it != qrds_status.end(); it++) {
        if(*it) {
            break;
        }
    }

    // Require four blacks in a row
    int count = 0;
    for (; it != qrds_status.end(); it++) {
        if(*it) {
            count++;
            if (count == 4) {
                cond = true;
                break;
            }
        }
        else {
            count = 0;
        }
    }
    return cond;

}

void IntersectionManager::handle_intersection() {

    switch (this->intersection_count) {
        case 0:
            this->drive_system->update(-0.1, -0.1);
            this->drive_system->actuate();
            delay(400);
            this->drive_system->update(-2.8, 0.98);
            this->drive_system->actuate();
            delay(400);
            this->drive_system->update(-0.1, -0.1);
            this->drive_system->actuate();
            delay(300);
            this->tape_sensor->set_state(MainTapeSensor::FAR_RIGHT);
            break;
        case 1:
            this->drive_system->update(0.94, 0.98);
            this->drive_system->actuate();
            delay(100);
            this->tape_sensor->set_state(MainTapeSensor::FAR_LEFT);
            break;
        case 2:
            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);
            this->drive_system->update(-3.0, -3.0);
            this->drive_system->actuate();
            delay(400);
            this->drive_system->update(0.93, -3.0);
            this->drive_system->actuate();
            delay(150);

            // get centered 
            center_post(true);
            
            this->drive_system->update(0.86, 0.86);
            this->drive_system->actuate();
            delay(350);

            //WIGGLE
            this->wiggle(12,120);

            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);

            grabCrystal();
            openClaw();

            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);
            this->drive_system->update(-3.0, -3.0);
            this->drive_system->actuate();
            delay(300);
            this->drive_system->update(-3.0, .98);
            this->drive_system->actuate();
            delay(300);
            this->tape_sensor->set_state(MainTapeSensor::FAR_RIGHT);
            break;
        case 3:
            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);
            this->drive_system->update(-3.0, -3.0);
            this->drive_system->actuate();
            delay(400);
            this->drive_system->update(0.93, -3.0);
            this->drive_system->actuate();
            delay(280);
            this->drive_system->update(0.86, 0.86);
            this->drive_system->actuate();
            delay(350);

            // WIGGLE
            this->wiggle(12,120);

            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);

            grabCrystal();
            
            this->drive_system->update(-3.0, -3.0);
            this->drive_system->actuate();
            delay(290);
            this->drive_system->update(.98, -3.0);
            this->drive_system->actuate();
            delay(500);
            this->tape_sensor->set_state(MainTapeSensor::FAR_LEFT);
            break;
            
        case 4:
            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);
            break;
        
        case 5:
            this->drive_system->update(0.0, 0.0);
            this->drive_system->actuate();
            delay(300);
            break;

        // Gauntlet here
        case 6:
            break;
    }
}
void IntersectionManager::handle_gauntlet() {

    switch(this->gauntlet_state) {

        case 0:

            // TODO: make the go into gauntlet code more reliable
            //    + more robust (works on various lipo voltages, angles, etc.)
            // Maybe we can rethink the algorithm/steps
            
            Serial.println("Initiating gauntlet sequence...");

            this->drive_system->update(-1.0, -1.0);
            this->drive_system->actuate();
            delay(300);
            
            this->drive_system->update(-3.5, 0.0);
            this->drive_system->actuate();

            this->tape_sensor->update();

            //TODO add failsafe timeout if we dont reach this condition so we dont drive off course
            while (!(this->tape_sensor->qrd3.is_on() || this->tape_sensor->qrd4.is_on())) {
                this->tape_sensor->update();
            }
            gauntlet_timer = millis();

            Serial.println("Followed back on tape");

            this->drive_system->set_speed_add(-0.04);

            this->gauntlet_state++;

            break;

        case 1:
            // keep pid going until picamera detects the gauntlet
            // TODO: incorporate timeout failsafe


            // if (millis() - gauntlet_timer >= 1600) {
            if (Serial.available() && Serial.read() == 'G') {

                Serial.println("Received G");

                this->gauntlet_state++;
                this->drive_system->set_speed_add(0.0);

                this->wiggle(10, 150);

                this->place_stone(0);

                // this->drive_system->update(0.0, 0.0);
                // this->drive_system->actuate();
                // delay(200);

                // this->drive_system->update(-3.0, -3.0);
                // this->drive_system->actuate();
                // delay(500);

                // this->tape_sensor->set_state(MainTapeSensor::FAR_LEFT);

                // gauntlet_timer = millis();

                // TODO:
                // drive back onto the course/place second stone
                delay(69420);

            }

            break;

        case 2:
            
            // keep pid going for ____ milliseconds and then increment to next state
            if (millis() - gauntlet_timer >= 1600) {

                this->drive_system->set_speed_add(0.0);

                this->gauntlet_state++;

                for (int i = 0; i < 16; i++) {
                    this->drive_system->update(0.93, -0.1);
                    this->drive_system->actuate();
                    delay(120);
                    this->drive_system->update(-0.1, 0.93);
                    this->drive_system->actuate();
                    delay(120);
                }

                this->drive_system->update(0.0, 0.0);
                this->drive_system->actuate();
                delay(500);

                this->drive_system->update(-3.5, 0.88);
                this->drive_system->actuate();
                delay(280);

                this->drive_system->update(0.0, 0.0);
                this->drive_system->actuate();
                delay(400);

                closeClaw();

                digitalWrite(STEPPERENABLE, LOW);
                moveZToExtreme(EXTEND);
                homeY(false);
                digitalWrite(STEPPERENABLE, HIGH);
                delay(1000);
                openClaw();
                digitalWrite(STEPPERENABLE, LOW);
                moveZToExtreme(EXTEND);
                homeY(true);
                moveZToExtreme(HOME);
                digitalWrite(STEPPERENABLE, HIGH);

                this->drive_system->update(0.94, 0.80);
                this->drive_system->actuate();
                delay(500);

                this->drive_system->update(0.0, 0.0);
                this->drive_system->actuate();
                delay(200);

                this->drive_system->update(0.88, -3.5);
                this->drive_system->actuate();
                delay(280);

                this->drive_system->update(0.0, 0.0);
                this->drive_system->actuate();
                delay(400);

                closeClaw();

                digitalWrite(STEPPERENABLE, LOW);
                moveZToExtreme(EXTEND);
                homeY(false);
                digitalWrite(STEPPERDIR, DOWN);
                for(int i = 0; i < 100; i++) {
                    stepperPulse();
                }
                digitalWrite(STEPPERENABLE, HIGH);

                delay(1000);
                openClaw();
                digitalWrite(STEPPERENABLE, LOW);
                moveZToExtreme(EXTEND);
                homeY(true);
                moveZToExtreme(HOME);
                digitalWrite(STEPPERENABLE, HIGH);

                delay(69420);
            }
            break;
    }
}

void IntersectionManager::place_stone(int slot) {

    // TODO: better algo could be to move forward y until close enough
    //  as y is moving, whenever x deviates too much, readjust x and then go forward in y

    // 1. Minimie x

    // initial values should fail the while loop checks
    int x = 999;
    int y = 999;

    Serial.println("Initiating deposit sequence...");

    bool complete = false;

    do {

        // Right now: turn left wheel back to hole 0

        // May be useful to find post when lost

        // ~20-25px per cm (crude estimate, height dependent, etc.)
        // currently have 0.05% pwm per px

        // turn left
        this->drive_system->update(-3.0, 0.4);
        this->drive_system->actuate();

        for (int i = 0; i < 150; i++) {

            if (Serial.read() == 'G') {

                x = Serial.readStringUntil(',').toInt();
                y = Serial.readStringUntil(';').toInt();

                if (fabs(x) < 20) {
                    complete = true;
                    break;
                }

            }

            delay(1);

        }

        if (complete) {
            break;
        }

        // Pause motors
        this->drive_system->update(0.0, 0.4);
        this->drive_system->actuate();

        for (int i = 0; i < 100; i++) {

            if (Serial.read() == 'G') {

                x = Serial.readStringUntil(',').toInt();
                y = Serial.readStringUntil(';').toInt();

                if (fabs(x) < 20) {
                    complete = true;
                    break;
                }

            }

            delay(1);

        }

    } while (fabs(x) > 20);

    this->drive_system->update(0.0, 0.0);
    this->drive_system->actuate();

    Serial.println("OMFG");

    // 3. Deposit stone
    depositCrystal();

}

// true turns the robot clockwise
void IntersectionManager::center_post(bool dir) {

    // TODO: better algo could be to move forward y until close enough
    //  as y is moving, whenever x deviates too much, readjust x and then go forward in y

    int mult = dir ? 1 : -1;

    // 1. Minimie x

    // initial values should fail the while loop checks
    int x = 999;
    int y = 999;

    Serial.println("Initiating post-centering sequence...");

    bool complete = false;

    do {

        // Right now: turn left wheel back to hole 0

        // May be useful to find post when lost

        // ~20-25px per cm (crude estimate, height dependent, etc.)
        // currently have 0.05% pwm per px

        // turn
        this->drive_system->update(mult * 3.0, -mult * 3.0);
        this->drive_system->actuate();

        for (int i = 0; i < 150; i++) {

            if (Serial.read() == 'P') {

                x = Serial.readStringUntil(',').toInt();
                y = Serial.readStringUntil(';').toInt();

                if (fabs(x) <= 30) {
                    complete = true;
                    break;
                }

            }

            delay(1);

        }

        if (complete) {
            break;
        }

        // Pause motors
        this->drive_system->update(0.0, 0.0);
        this->drive_system->actuate();

        for (int i = 0; i < 100; i++) {

            if (Serial.read() == 'P') {

                x = Serial.readStringUntil(',').toInt();
                y = Serial.readStringUntil(';').toInt();

                if (fabs(x) <= 30) {
                    complete = true;
                    break;
                }

            }

            delay(1);

        }

    } while (fabs(x) > 30);

    this->drive_system->update(0.0, 0.0);
    this->drive_system->actuate();

    Serial.println("YOMFG");

}

