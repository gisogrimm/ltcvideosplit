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
