// Filter.h - Arduino library to Filter Sensor Data
// Copyright 2012 Jeroen Doggen (jeroendoggen@gmail.com)
// For more information: variable declaration, changelog,... see Filter.h
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/// SignalFilter - Library to Filter Sensor Data using digital filters
/// Available filters: Chebyshev & Bessel low pass filter (1st & 2nd order)

#include <Arduino.h>
#include <SignalFilter.h>

/// Constructor
SignalFilter::SignalFilter()
{
  _v[0]=0;
  _v[1]=0;
  _v[2]=0;
  _helper=0; 
  _counter=0;
  _filter=0;
  _order=0;
  _average=0;
  _median=0;
}

/// Begin function: set default filter options
void SignalFilter::begin()
{
  setFilter('c');
  setOrder(1);
}

/// setFilter(char filter): Select filter: 'c' -> Chebyshev, 'b' -> Bessel
void SignalFilter::setFilter(char filter)
{
  _filter=filter;
}

/// selectOrder(int order): Select filter order (1 or 2)
void SignalFilter::setOrder(int order)
{
  _order=order;
}

///
void SignalFilter::printSamples()
{
  Serial.print(" ");
  Serial.print(_v[2]);

  Serial.print(" ");
  Serial.print(_v[1]);

  Serial.print(" ");
  Serial.print(_v[0]);
  Serial.print(" - ");
}

/// filter: Runs the actual filter: input=rawdata, output=filtered data
int SignalFilter::run(int data)
{
//  Uncomment for debugging
//  Serial.println(_filter);
//  Serial.println(_order);
/// Chebyshev filters
  if(_filter=='c')
  {
    if(_order==1)                                 //ripple -3dB
    {
      _v[0] = _v[1];
      long tmp = ((((data * 3269048L) >>  2)      //= (3.897009118e-1 * data)
        + ((_v[0] * 3701023L) >> 3)               //+(  0.2205981765*v[0])
        )+1048576) >> 21;                         // round and downshift fixed point /2097152
      _v[1]= (short)tmp;
      return (short)(_v[0] + _v[1]);              // 2^
    }
    if(_order==2)                                 //ripple -1dB
    {
      _v[0] = _v[1];
      _v[1] = _v[2];
      long tmp = ((((data * 662828L) >>  4)       //= (    7.901529699e-2 * x)
        + ((_v[0] * -540791L) >> 1)               //+( -0.5157387562*v[0])
        + (_v[1] * 628977L)                       //+(  1.1996775682*v[1])
        )+262144) >> 19;                          // round and downshift fixed point /524288

      _v[2]= (short)tmp;
      return (short)((
        (_v[0] + _v[2])
        +2 * _v[1]));                             // 2^
    }
  }

/// Bessel filters
  else if(_filter=='b')                                // Bessel filters
  {
    if(_order==1)                                 //Alpha Low 0.1
    {
      _v[0] = _v[1];
      long tmp = ((((data * 2057199L) >>  3)      //= (    2.452372753e-1 * x)
        + ((_v[0] * 1068552L) >> 1)               //+(  0.5095254495*v[0])
        )+524288) >> 20;                          // round and downshift fixed point /1048576
      _v[1]= (short)tmp;
      return (short)(((_v[0] + _v[1])));          // 2^
    }
    if(_order==2)                                 //Alpha Low 0.1
    {
      _v[0] = _v[1];
      _v[1] = _v[2];
      long tmp = ((((data * 759505L) >>  4)       //= (    9.053999670e-2 * x)
        + ((_v[0] * -1011418L) >> 3)              //+( -0.2411407388*v[0])
        + ((_v[1] * 921678L) >> 1)                //+(  0.8789807520*v[1])
        )+262144) >> 19;                          // round and downshift fixed point /524288

      _v[2]= (short)tmp;
      return (short)(((_v[0] + _v[2])+2 * _v[1]));// 2^
    }
  }

/// Median filters (78 bytes, 12 microseconds)
  else if(_filter=='m')                                // New filters
  {
// Note:
//  quick & dirty dumb implementation that only keeps 3 samples: probably better to do insertion sort when more samples are needed in the calculation
//   or Partial sort: http://en.cppreference.com/w/cpp/algorithm/nth_element
// On better inspection of this code... performance seem quite good
// TODO: compare with: http://embeddedgurus.com/stack-overflow/tag/median-filter/
    _v[0] = _v[1];
    _v[1] = _v[2];
    _v[2]= data;

//       printSamples();

    if (_v[2] < _v[1])
    {
      if (_v[2] < _v[0])
      {
        if (_v[1] < _v[0])
        {
          _median = _v[1];
        }
        else
        {
          _median = _v[0];
        }
      }
      else
      {
        _median = _v[2];
      }
    }
    else
    {
      if (_v[2] < _v[0])
      {
        _median = _v[2];
      }
      else
      {
        if (_v[1] < _v[0])
        {
          _median = _v[0];
        }
        else
        {
          _median = _v[1];
        }
      }
    }
    return (_median);
  }

//
/// Median filter (148 bytes, 12 microseconds)
// less efficient, but more readable?
  else if(_filter=='n')
  {
    _v[0] = _v[1];
    _v[1] = _v[2];
    _v[2]= data ;

//printSamples();

    if( ((_v[2] < _v[1]) && (_v[2] > _v[0])) ||(( _v[2] < _v[0]) && (_v[2] > _v[1])))
      return (_v[2]);

    if( (_v[1] < _v[2] && _v[1] > _v[0]) ||( _v[1] < _v[0] && _v[1] > _v[2]))
      return (_v[1]);

    if( (_v[0] < _v[2] && _v[0] > _v[1]) ||( _v[0] < _v[1] && _v[0] > _v[2]))
      return (_v[0]);
  }
  
/// Median filter (78 bytes, 12 microseconds)
// based on: http://embeddedgurus.com/stack-overflow/tag/median-filter/
// same code size as my median filter code

  else if(_filter=='0')
  {
    _v[0] = _v[1];
    _v[1] = _v[2];
    _v[2]= data ;
    
    int middle;

    if ((_v[0] <= _v[1]) && (_v[0] <= _v[2]))
    {
      middle = (_v[1] <= _v[2]) ? _v[1] : _v[2];
    }
    else if ((_v[1] <= _v[0]) && (_v[1] <= _v[2]))
    {
      middle = (_v[0] <= _v[2]) ? _v[0] : _v[2];
    }
    else
    {
      middle = (_v[0] <= _v[1]) ? _v[0] : _v[1];
    }
    return middle;
  }
  
//
/// Growing-shrinking filter (fast)
  else if(_filter=='g')                                // New filters
  {
    if (data > _helper)
    {
      if (data > _helper+512)
        _helper=_helper+512;
      if (data > _helper+128)
        _helper=_helper+128;
      if (data > _helper+32)
        _helper=_helper+32;
      if (data > _helper+8)
        _helper=_helper+8;
      _helper++;
    }
    else if (data < _helper)
    {
      if (data < _helper-512)
        _helper=_helper-512;
      if (data < _helper-128)
        _helper=_helper-128;
      if (data < _helper-32)
        _helper=_helper-32;
      if (data < _helper-8)
        _helper=_helper-8;
      _helper--;
    }
    return _helper;
  }
/// Growing-shrinking filter (smoother)
  else if(_filter=='h')                                // New filters
  {
    if (data > _helper)
    {
      if (data > _helper+8)
      {
        _counter++;
        _helper=_helper + 8 * _counter;
      }
      _helper++;
    }
    else if (data < _helper)
    {
      if (data < _helper-8)
      {
        _counter++;
        _helper=_helper- 8 * _counter;
      }
      _helper--;
    }

    if (_counter > 10)
    {
      _counter=0;
    }
    Serial.print(" counter: ");
    Serial.print(_counter);
    Serial.print("  ");
    return _helper;
  }
  return 0; //should never be reached
}
