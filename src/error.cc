#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

error_msg_t::error_msg_t(const char* s_file,int l,const char* fmt,...)
{
  int len(0);
  va_list ap;
  va_start(ap,fmt);
  char * dummy_str = 0;
  len = vsnprintf(dummy_str,0,fmt,ap);
  len += strlen(s_file)+10;
  msg = new char[len+1];
  snprintf(msg,len+1,"%s:%d: ",s_file,l);
  uint32_t flen(strlen(msg));
  dummy_str = &(msg[flen]);
  va_end(ap);
  va_start(ap,fmt);    
  vsnprintf(dummy_str,len+1-flen,fmt,ap);
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
