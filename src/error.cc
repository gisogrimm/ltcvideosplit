/*
    error handling class implementation
    Copyright (C) 2016 Giso Grimm

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

error_msg_t::error_msg_t(const char* srcfile,int srcline,const char* fmt,...)
{
  int len(0);
  va_list ap;
  va_start(ap,fmt);
  char* ctmp = 0;
  len = vsnprintf(ctmp,0,fmt,ap);
  len += strlen(srcfile)+10;
  msg = new char[len+1];
  snprintf(msg,len+1,"%s:%d: ",srcfile,srcline);
  uint32_t flen(strlen(msg));
  ctmp = &(msg[flen]);
  va_end(ap);
  va_start(ap,fmt);    
  vsnprintf(ctmp,len+1-flen,fmt,ap);
  msg[len] = '\0';
  va_end(ap);
}

error_msg_t::~error_msg_t() throw ()
{
  delete [] msg;
}

error_msg_t& error_msg_t::operator=(const error_msg_t& p)
{
  if( msg )
    delete [] msg;
  int len;
  len = strlen(p.msg);
  msg = new char[len+1];
  memset(msg,0,len+1);
  strncpy(msg,p.msg,len);
  return *this;
}

error_msg_t::error_msg_t(const error_msg_t& p)
{
  int len;
  len = strlen(p.msg);
  msg = new char[len+1];
  memset(msg,0,len+1);
  strncpy(msg,p.msg,len);
}

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
