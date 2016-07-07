/*
    error handling class definition
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
#ifndef ERROR_H
#define ERROR_H

#include <exception>

class error_msg_t : public std::exception {
public:
  error_msg_t(const char* file,int line,const char* fmt,...);
  error_msg_t(const error_msg_t&);
  error_msg_t& operator=(const error_msg_t&);
  ~error_msg_t() throw ();
  const char* what() const throw () { return msg; };
private:
  char* msg;
};


#endif

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
