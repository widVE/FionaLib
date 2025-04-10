// -*- Mode:C++ -*-

/*
 *  ad-box driver
 *  works with Fraunhofer IMK AD-Box and Fakespace Cubic Mouse
 *
 *  for additional information see:
 *  http://www.imk.fraunhofer.de
 *  http://www.fakespace.com
 *
 *  written by Sascha Scholz <sascha.scholz@imk.fraunhofer.de>
 */

#ifndef VRPN_ADBOX_H
#define VRPN_ADBOX_H

#include "vrpn_Analog.h"                // for vrpn_Analog
#include "vrpn_Button.h"                // for VRPN_BUTTON_BUF_SIZE, etc
#include "vrpn_Configure.h"             // for VRPN_API
#include "vrpn_Shared.h"                // for timeval

class VRPN_API vrpn_Connection;

class VRPN_API vrpn_ADBox : public vrpn_Analog, public vrpn_Button_Filter {

 public:
  vrpn_ADBox(char* name, vrpn_Connection *c,
             const char *port="/dev/ttyS1/", long baud=9600);

  ~vrpn_ADBox();

  void mainloop();
   
 private:
  int ready;

  struct timeval timestamp;	// time of the last report from the device
  
  int serial_fd;
  unsigned char buffer[VRPN_BUTTON_BUF_SIZE];
  
  int iNumBytes;
  int iNumDigBytes;
  int iFilter[8][30];
  int iFilterPos;
};

#endif // #ifndef VRPN_ADBOX
