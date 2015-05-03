//
//  LuaDebugger_LuaBreakpoint.hpp
//  BaseStation
//
//  Created by Mark Pauley on 7/14/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//  INTERNAL TO LuaDebugger.h!!

#ifndef UTIL_LUA_LUADEBUGGER_LUABREAKPOINT_HPP_
#define UTIL_LUA_LUADEBUGGER_LUABREAKPOINT_HPP_

///////////////
// Breakpoints
///////////////
class LuaBreakpoint {
public:
  LuaBreakpoint()
  : _hitCount(0) {};
  
  LuaBreakpoint(const LuaBreakpoint& other)
  : _hitCount(other._hitCount) {};
  
  virtual ~LuaBreakpoint() {};
  
  virtual bool operator ==(const LuaBreakpoint& other) const = 0;
  
  void Hit() const
  {
    _hitCount++;
  }
  
  unsigned int GetHitCount() const
  {
    return _hitCount;
  };
  
private:
  unsigned mutable int _hitCount;
};

#pragma mark Function breakpoints
class LuaBreakpoint_Function : public LuaBreakpoint {
public:
  LuaBreakpoint_Function(const std::string& functionName)
  : LuaBreakpoint()
  , _functionName(functionName) {};
  
  bool operator ==(const LuaBreakpoint& other) const
  {
    const LuaBreakpoint_Function* otherFuncBP = dynamic_cast<const LuaBreakpoint_Function*>(&other);
    if(otherFuncBP != nullptr) {
      return _functionName == otherFuncBP->_functionName;
    }
    return false;
  }
  
  bool operator <(const LuaBreakpoint_Function& other) const {
    return _functionName < other._functionName;
  }
  
private:
  const std::string _functionName;
};

#pragma mark Line number breakpoints
class LuaBreakpoint_Line : public LuaBreakpoint {
public:
  LuaBreakpoint_Line(const std::string& fileName, const int lineNumber)
  : LuaBreakpoint()
  , _fileName(fileName)
  , _lineNumber(lineNumber) {};
  
  bool operator ==(const LuaBreakpoint& other) const
  {
    const LuaBreakpoint_Line* otherLineBP = dynamic_cast<const LuaBreakpoint_Line*>(&other);
    if(otherLineBP != nullptr) {
      const bool ret = (_fileName == otherLineBP->_fileName);
      if(ret) {
        return _lineNumber == otherLineBP->_lineNumber;
      }
    }
    return false;
  }
  
  bool operator <(const LuaBreakpoint_Line& other) const
  {
    int comparison = _fileName.compare(other._fileName);
    if(comparison < 0) return true;
    else if (comparison > 0) return false;
    else return (_lineNumber < other._lineNumber);
  }
  
  static LuaBreakpoint_Line AnyLine() { return LuaBreakpoint_Line("ANY", -1); }
  
private:
  const std::string _fileName;
  const int _lineNumber;
};


#endif
