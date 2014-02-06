#include "DAQMXclass.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(this->error=(functionCall)) ) goto Error; else

char errBuff[2048]={'\0'};

DAQMX::DAQMX(char *portName)
{
	uInt8 dc[8] = {Nsamp/2, Nsamp/2, Nsamp/2, Nsamp/2, Nsamp/2, Nsamp/2, Nsamp/2, Nsamp/2};

    //We're not yet connected
    connected = false;

	// initialize error
	error = 0;

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	taskHandle = 0;
	DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
	DAQmxErrChk (DAQmxCreateDOChan(taskHandle,portName,"",DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"",Fclk,DAQmx_Val_Rising,DAQmx_Val_ContSamps,Nsamp));

	/*********************************************/
	// DAQmx Create initial waveform buffer
	/*********************************************/
	createPWM(dc);			// updates DAQdata

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxWriteDigitalU8(taskHandle,Nsamp,0,10.0,DAQmx_Val_GroupByChannel,DAQdata,NULL,NULL));
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	connected = true;

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		printf("DAQmx Error: %s\n",errBuff);
	}
}

DAQMX::~DAQMX()
{
    //Check if we are connected before trying to disconnect
    if(this->connected)
    {
        //We're no longer connected
        this->connected = false;
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(this->taskHandle);
		DAQmxClearTask(this->taskHandle);
    }
}

bool DAQMX::IsConnected()
{
    //Simply return the connection status
    return this->connected;
}

void DAQMX::writePWM(uInt8 dc[8])
{
	error = 0;

	createPWM(dc);
	DAQmxErrChk (DAQmxWriteDigitalU8(taskHandle,Nsamp,0,10.0,DAQmx_Val_GroupByChannel,DAQdata,NULL,NULL));

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		printf("DAQmx Error: %s\n",errBuff);
	}
}

void DAQMX::createPWM(uInt8 dc[8])
{
	for(int i=0;i<Nsamp;i++) {
		DAQdata[i] = 0;
		for(int j=7;j>=0;j--) {
			DAQdata[i] <<= 1;
			DAQdata[i] += (i < dc[j]) ? 1 : 0;
		}
	}
}