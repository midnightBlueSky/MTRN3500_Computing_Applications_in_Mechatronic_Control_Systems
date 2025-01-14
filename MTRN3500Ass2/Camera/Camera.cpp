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

	/*Create the actual shared memory space for each SMobject by calling member function */
	//PMObj.SMCreate();

	/*Get access from this process to shared memory for PM and Xbox (needed for shutdown?) */
	PMObj.SMAccess();

	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;

	int PMFailCount = 0; 
	while (!PM->Shutdown.Flags.Camera)
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
		PM->Heartbeat.Flags.Camera = 1;
		std::cout << "Camera is running" << std::endl;
	}
}




/*

Camera Lecture Due Week 13





*/