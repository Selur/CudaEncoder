/*
 * MyTimer.cpp
 *
 *  Created on: 02.12.2010
 *      Author: Selur
 */

#include "MyTimer.h"


MyTimer::MyTimer()
: start_time(), end_time(), diff_time(0.0), total_time(0.0), clock_sessions(0), running(false)
{
}

MyTimer::~MyTimer()
{
}

void MyTimer::start()
{
    this->start_time = QTime::currentTime();
    this->running = true;
}

//! Stop time measurement
void MyTimer::stop()
{
    this->end_time = QTime::currentTime();
    this->diff_time = this->start_time.msecsTo(this->end_time);
    this->total_time += this->diff_time;
    this->clock_sessions++;
    this->running = false;
}

//! Reset time counters to zero
void MyTimer::reset()
{
    this->diff_time = 0;
    this->total_time = 0;
    this->clock_sessions = 0;
    if (running) {
        this->start_time = QTime::currentTime();
    }
}

//! Time in msec. after start. If the stop watch is still running (i.e. there
//! was no call to stop()) then the elapsed time is returned, otherwise the
//! time between the last start() and stop call is returned
const float MyTimer::getTime() const
{
    // Return the TOTAL time to date
    float retval = this->total_time;
    if (this->running) {
        retval += this->start_time.msecsTo(QTime::currentTime());
    }

    return retval;
}

//! Mean time to date based on the number of times the stopwatch has been
//! _stopped_ (ie finished sessions) and the current total time
const float MyTimer::getAverageTime() const
{
    return (this->clock_sessions > 0) ? (this->total_time/this->clock_sessions) : 0.0;
}
