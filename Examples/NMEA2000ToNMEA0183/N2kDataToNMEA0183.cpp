/*
N2kDataToNMEA0183.cpp

Copyright (c) 2015-2018 Timo Lappalainen, Kave Oy, www.kave.fi

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "N2kDataToNMEA0183.h"
#include <N2kMessages.h>
#include <NMEA0183Messages.h>

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleMsg(const tN2kMsg &N2kMsg) {
  switch (N2kMsg.PGN) {
    case 127250UL: HandleHeading(N2kMsg);
    case 127258UL: HandleVariation(N2kMsg);
    case 128259UL: HandleBoatSpeed(N2kMsg);
    case 128267UL: HandleDepth(N2kMsg);
    case 129025UL: HandlePosition(N2kMsg);
    case 129026UL: HandleCOGSOG(N2kMsg);
    case 129029UL: HandleGNSS(N2kMsg);
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::Update() {
  SendRMC();
  if ( LastHeadingTime+2000<millis() ) Heading=N2kDoubleNA;
  if ( LastCOGSOGTime+2000<millis() ) { COG=N2kDoubleNA; SOG=N2kDoubleNA; }
  if ( LastPositionTime+4000<millis() ) { Latitude=N2kDoubleNA; Longitude=N2kDoubleNA; }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleHeading(const tN2kMsg &N2kMsg) {
unsigned char SID;
tN2kHeadingReference ref;
double _Deviation=0;
double _Variation;
tNMEA0183Msg NMEA0183Msg;

  if ( ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, _Variation, ref) ) {
    if ( ref==N2khr_magnetic ) {
      if ( !N2kIsNA(_Variation) ) Variation=_Variation; // Update Variation
      if ( !N2kIsNA(Heading) && !N2kIsNA(Variation) ) Heading-=Variation;
    }
    LastHeadingTime=millis();
    if ( NMEA0183SetHDG(NMEA0183Msg,Heading,_Deviation,Variation) ) {
      pNMEA0183->SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleVariation(const tN2kMsg &N2kMsg) {
unsigned char SID;
tN2kMagneticVariation Source;

  ParseN2kMagneticVariation(N2kMsg,SID,Source,DaysSince1970,Variation);
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleBoatSpeed(const tN2kMsg &N2kMsg) {
unsigned char SID;
double WaterReferenced;
double GroundReferenced;
tN2kSpeedWaterReferenceType SWRT;

  if ( ParseN2kBoatSpeed(N2kMsg,SID,WaterReferenced,GroundReferenced,SWRT) ) {
    tNMEA0183Msg NMEA0183Msg;
    double MagneticHeading=( !N2kIsNA(Heading) && !N2kIsNA(Variation)?Heading+Variation: NMEA0183DoubleNA);
    if ( NMEA0183SetVHW(NMEA0183Msg,Heading,MagneticHeading,WaterReferenced) ) {
      pNMEA0183->SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleDepth(const tN2kMsg &N2kMsg) {
unsigned char SID;
double DepthBelowTransducer;
double Offset;
double Range;

  if ( ParseN2kWaterDepth(N2kMsg,SID,DepthBelowTransducer,Offset,Range) ) {
      tNMEA0183Msg NMEA0183Msg;
      if ( NMEA0183SetDPT(NMEA0183Msg,DepthBelowTransducer,Offset) ) {
        pNMEA0183->SendMessage(NMEA0183Msg);
      }
      if ( NMEA0183SetDBx(NMEA0183Msg,DepthBelowTransducer,Offset) ) {
        pNMEA0183->SendMessage(NMEA0183Msg);
      }
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandlePosition(const tN2kMsg &N2kMsg) {

  if ( ParseN2kPGN129025(N2kMsg, Latitude, Longitude) ) {
    LastPositionTime=millis();
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleCOGSOG(const tN2kMsg &N2kMsg) {
unsigned char SID;
tN2kHeadingReference HeadingReference;
tNMEA0183Msg NMEA0183Msg;

  if ( ParseN2kCOGSOGRapid(N2kMsg,SID,HeadingReference,COG,SOG) ) {
    LastCOGSOGTime=millis();
    double MCOG=( !N2kIsNA(COG) && !N2kIsNA(Variation)?COG-Variation:NMEA0183DoubleNA );
    if ( HeadingReference==N2khr_magnetic ) {
      MCOG=COG;
      if ( !N2kIsNA(Variation) ) COG-=Variation;
    }
    if ( NMEA0183SetVTG(NMEA0183Msg,COG,MCOG,SOG) ) {
      pNMEA0183->SendMessage(NMEA0183Msg);
    }
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::HandleGNSS(const tN2kMsg &N2kMsg) {
unsigned char SID;
tN2kGNSStype GNSStype;
tN2kGNSSmethod GNSSmethod;
unsigned char nSatellites;
double HDOP;
double PDOP;
double GeoidalSeparation;
unsigned char nReferenceStations;
tN2kGNSStype ReferenceStationType;
uint16_t ReferenceSationID;
double AgeOfCorrection;

  if ( ParseN2kGNSS(N2kMsg,SID,DaysSince1970,SecondsSinceMidnight,Latitude,Longitude,Altitude,GNSStype,GNSSmethod,
                    nSatellites,HDOP,PDOP,GeoidalSeparation,
                    nReferenceStations,ReferenceStationType,ReferenceSationID,AgeOfCorrection) ) {
    LastPositionTime=millis();
  }
}

//*****************************************************************************
void tN2kPosInfoToNMEA0183::SendRMC() {
    if ( NextRMCSend<=millis() && !N2kIsNA(Latitude) ) {
      tNMEA0183Msg NMEA0183Msg;
      if ( NMEA0183SetRMC(NMEA0183Msg,SecondsSinceMidnight,Latitude,Longitude,COG,SOG,DaysSince1970,Variation) ) {
        pNMEA0183->SendMessage(NMEA0183Msg);
      }
      SetNextRMCSend();
    }
}

