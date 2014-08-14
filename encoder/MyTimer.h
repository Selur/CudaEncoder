/*
 * MyTimer.h
 *
 *  Created on: 02.12.2010
 *      Author: Selur
 */

#ifndef MYTIMER_H_
#define MYTIMER_H_
#include <QTime>
class MyTimer
{
    public:
        MyTimer();
        virtual ~MyTimer();

        //! Start time measurement
        void start();

        //! Stop time measurement
        void stop();

        //! Reset time counters to zero
        void reset();

        //! Time in msec. after start. If the stop watch is still running (i.e. there
        //! was no call to stop()) then the elapsed time is returned, otherwise the
        //! time between the last start() and stop call is returned
        const float getTime() const;

        //! Mean time to date based on the number of times the stopwatch has been
        //! _stopped_ (ie finished sessions) and the current total time
        const float getAverageTime() const;

    private:
        QTime start_time, end_time;
        double diff_time, total_time;
        int clock_sessions;
        bool running;
};

#endif /* MYTIMER_H_ */
