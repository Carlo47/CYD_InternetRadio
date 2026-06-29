#include "Wait.h"

void Wait::begin(uint32_t msWait, int nTimes)
{
  _msWait      = msWait;
  _nTimes      = nTimes;
  _msPrevious  = millis();
  _auto_reload = (nTimes <= 0);
}

bool Wait::isOver()
{
  uint32_t dt = millis() - _msPrevious;
  if (!_done)
  {
    if (dt > _msWait)
    {
      if (_auto_reload)
      {
        _msPrevious = millis(); // reload
        return true;
      }
      if (_nTimes > 0)
      {
        _nTimes--;
        _msPrevious = millis(); // reload
        return true;
      }
      else
      {
        _done = true;
      }
    }
  }
  return false;
}

void Wait::msWaitSet(uint32_t msWait)
{
  _msWait = msWait;
}