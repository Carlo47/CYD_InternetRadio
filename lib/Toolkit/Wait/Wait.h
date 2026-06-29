#pragma once
#include <Arduino.h>

/**
 * Class    Wait
 * Author   2026-06-14 Charles Geiser https://github.com/Carlo47
 *
 * Purpose  A simple class that checks, in a non-blocking manner,
 *          whether a specified amount of time in milliseconds has elapsed.
 *          Once the wait time has elapsed, the isOver() function
 *          returns true.
 *          If the second argument is not specified when calling begin(),
 *          the wait time is automatically reloaded and isOver() returns
 *          true periodically.
 *          However, if a positive value N is specified as the second
 *          argument, the isOver() function returns true only N times.
 *
 * Usage    Wait waitForSomething;
 *          Wait waitForSomeThingNtimes;
 *
 *          void doSomeThing()
 *          {...}
 *
 *          void doSomeThingNtimes()
 *          {...}
 *
 *          void setup()
 *          {
 *            // Start the timers
 *            waitForSomething.begin(5000);         // do it every 5000 ms
 *            waitForSomeThingOnce.begin(12500, 3); // do it 3 times every 12.5 sec
 *          }
 *          void loop()
 *          {
 *            if (waitForSomething.isOver()) doSomething();
 *            if (waitForSomeThingNtimes.isOver()) doSomeThingNtimes();
 *            ...
 *          }
 */

class Wait
{
  public:
    Wait() = default;
    void begin(uint32_t msWait, int nTimes = 0);
    bool isOver();
    void msWaitSet(uint32_t msWait);

  private:
    uint32_t _msWait;
    int      _nTimes;
    bool     _done        = false;
    bool     _auto_reload = true;
    uint32_t _msPrevious  = 0;
};