#include <sndfile.h>
#include <string.h>
#include <iostream>
#include "error.h"

#define PR(x) std::cout << #x << ": " << bcinfo.x << std::endl

int main(int argc, char** argv)
{
  try{
    if( argc < 2 )
      throw error_msg_t(__FILE__,__LINE__,"Usage: %s sndfilename",argv[0]);
    SF_INFO info;
    memset(&info,0,sizeof(info));
    SNDFILE* sf(sf_open(argv[1],SFM_READ,&info));
    if( !sf )
      throw error_msg_t(__FILE__,__LINE__,"Unable to open file \"%s\".",argv[1]);
    SF_BROADCAST_INFO bcinfo;
    if( !sf_command(sf,SFC_GET_BROADCAST_INFO,&bcinfo,sizeof(bcinfo)) )
      throw error_msg_t(__FILE__,__LINE__,"The file \"%s\" does not contain a Broadcast Extension chunk.",argv[1]);
    int sec(bcinfo.time_reference_low/info.samplerate);
    int msec((1000*(bcinfo.time_reference_low % info.samplerate))/info.samplerate);
    char stime[32];
    memset(stime,0,32);
    sprintf( stime, "%02d:%02d:%02d.%03d (%d)",sec/3600,(sec/60)%60,sec%60,msec,(25*bcinfo.time_reference_low)/info.samplerate);
    std::cout << stime << std::endl;
    //PR(time_reference_high);
  } 
  catch( const std::exception& e){
    std::cerr << "Error:\n" << e.what() << std::endl;
    return 1;
  }
  return 0;
}

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
