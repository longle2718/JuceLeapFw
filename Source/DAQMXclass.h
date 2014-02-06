#ifndef DAQMXCLASS_H_INCLUDED
#define DAQMXCLASS_H_INCLUDED

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <NIDAQmx.h>

#define Nsamp	256
#define Fclk	Nsamp*30000 // must be less than 10Mhz, which is max clock supported by PCIe-6535

class DAQMX
{
    private:
        //DAQmx task handle
       	TaskHandle taskHandle;
		//DAQmx u8 waveform
		uInt8 DAQdata[Nsamp];
        //Connection status
        bool connected;
        //Keep track of last error
        int32 error;
		void createPWM(uInt8 dc[8]);

    public:
        //Initialize Serial communication with the given COM port
        DAQMX(char *portName);
        //Close the connection
        //NOTA: for some reason you can't connect again before exiting
        //the program and running it again
        ~DAQMX();
        //Check if we are actually connected
        bool IsConnected();
		// [d]uty [c]ycle has 8 values between 0 and 255
		void writePWM(uInt8 dc[8]);
};

#endif // SERIALCLASS_H_INCLUDED