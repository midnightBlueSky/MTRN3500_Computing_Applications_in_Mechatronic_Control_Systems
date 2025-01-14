#include "pch.h"
#include <sstream>
#include <iostream>
#include <Windows.h>
#include <SMObject.h>
#include <SMStruct.h>

int main()
{
	

	/*Declare Shared Memory objects for process managent, gps
	as defined in SMStruct.h */
	TCHAR PMT[] = TEXT("ProcessManagement");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	TCHAR PLOT[] = TEXT("Plotting"); 
	SMObject PlotObj(PLOT, sizeof(Plotting)); 
	/*Create the actual shared memory space for each SMobject by calling member function */
	//PMObj.SMCreate();

	/*Get access from this process to shared memory for PM and Xbox (needed for shutdown?) */
	PMObj.SMAccess();
	PlotObj.SMAccess(); 
	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	Plotting * pPlot = (Plotting*)PlotObj.pData; 

	__int64 Frequency, HPCCount;
	double PlotTimeStamp;
	QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency); //Get HPC Frequency
	int PMFailCount = 0;
	while (!PM->Shutdown.Flags.Visualisation)
	{
		if (!PM->Heartbeat.Flags.PM)	//check process management is still running 
		{
			PMFailCount++;
			if (PMFailCount > 200)
			{
				PMFailCount = 0; 
				break;
			}
		}
		else PMFailCount = 0;
			

		QueryPerformanceCounter((LARGE_INTEGER*)&HPCCount); //Get HPC Count 
		PlotTimeStamp = (double)HPCCount / (double)Frequency;//Calculate TimeStamp
		pPlot->PlotTimeStamp = PlotTimeStamp; 
		PM->Heartbeat.Flags.Visualisation= 1;
		std::cout << "Visualisation timestamp: " << pPlot->PlotTimeStamp << std::endl;
	}
}

