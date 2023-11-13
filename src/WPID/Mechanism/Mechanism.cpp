#include "WPID/Mechanism/Mechanism.h"
#include <stdlib.h>
using namespace vex;
using namespace wpid;

Mechanism::Mechanism(motor_group* motors, float gear_ratio, std::string mech_id){
    this->motors = motors;
    this->gear_ratio = gear_ratio;
    this->mech_id = mech_id;
}

void Mechanism::spin(int velocity){
    float position = this->getPosition(rotationUnits::deg);
    if((velocity > 0 && position < upper_bound)
    || (velocity < 0 && position > lower_bound)
    || (upper_bound == lower_bound)){
        motors->spin(directionType::fwd, velocity, velocityUnits::pct);
    } else {
        motors->stop();
    }
}

void Mechanism::stop(){
    motors->stop();
}

void Mechanism::waitUntilSettled(){
    while(!this->isSettled){
        wait(20, msec);
    }
    LOG("stopping mech");
}

void Mechanism::moveRelativeAsync(float position, float max_speed){
    LOG("entering mech relative async");
    float current = this->getPosition(deg);
    this->moveAbsoluteAsync(position + current, max_speed);
    LOG("exiting mech relative async");

}

void Mechanism::moveRelative(float position, float max_speed){
    LOG("entering mech relative");
    this->moveRelativeAsync(position, max_speed);
    this->waitUntilSettled();
    LOG("exiting mech relative");
}

void Mechanism::moveAbsoluteAsync(float position, float max_speed){
    LOG("entering mech absolute async");
    isSettled = false;  
    void* arg = (void*)getParams(position, max_speed);
    thread(setTarget, arg);
    LOG("exiting mech absolute async");

}

void Mechanism::moveAbsolute(float position, float max_speed){
    LOG("entering mech absolute");
    this->moveAbsoluteAsync(position, max_speed);
    this->waitUntilSettled();
    LOG("exiting mech absolute");

}

void Mechanism::setTarget(void* args){
    LOG("entering mech set target");
    params* input = (params*)args;
    Mechanism* mech = input->mech;
    float max_speed = input->spd;
    float target = (input->pos + mech->offset);

    //limit target to bounds if calcluations exceed bounds
    if(target > mech->upper_bound){
        target = mech->upper_bound;
    } else if (target < mech->lower_bound) {
        target = mech->lower_bound;
    }

    float state = 0;
    float error = 999;
    int calc = 0;
    float ramp = 0;
    
    while(mech->pid.cont(error)){
        state = mech->getPosition(rotationUnits::deg); // get the state of the motors
        error = target - state; // difference between target and state
        //calc = mech->pid.calculateSpeed(error, max_speed); // calculate PID speed
        calc = mech->pid.calculateSpeed(error, max_speed, mech->mech_id); // calculate PID speed
        // if(mech->max_acceleration > 0 && fabs(error) > fabs(target*(1 - mech->MAX_RAMP_DURATION)) && ramp <= max_speed){
        //     calc = target < 0 ? 0.0 - ramp : ramp;
        //     ramp += mech->max_acceleration;
        // }

        mech->spin(calc); // spin the motors at speed
        this_thread::sleep_for(mech->pid.delay_time); // delay by pid.delay_time milliseconds
    }
    mech->pid.reset();
    mech->stop();
    mech->isSettled = true;
    LOG("exiting mech set target");
    return;
}

float Mechanism::getPosition(rotationUnits units){
    return motors->position(units) * gear_ratio;
}

void Mechanism::resetPosition(){
    motors->resetPosition();
}

void Mechanism::setBrakeType(brakeType type){
    motors->setStopping(type);
}

void Mechanism::setPID(PID pid){
    this->pid = pid;
}

void Mechanism::setOffset(float offset){
    this->offset = offset;
}

void Mechanism::setMaxAcceleration(float max_accel){
    this->max_acceleration = max_accel;
}

void Mechanism::setBounds(float lower_bound, float upper_bound){
    this->lower_bound = lower_bound;
    this->upper_bound = upper_bound;
}
