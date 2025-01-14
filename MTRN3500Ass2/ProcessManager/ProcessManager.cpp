#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <TlHelp32.h>
#include <SMObject.h>
#include <SMStruct.h>
#include <tchar.h>
#include <stdio.h>

using namespace std;	//myver: |Unused|Unused:Visualisation:Remote:VehicleControl|Camera:GPS:Laser:PM|
//#define CRITICAL_MASK 0x0017		//0000 0000 0001 0111		//myver:xxxx xxxx x010 1011 
//#define NONCRITICAL_MASK 0x0008		//0000 0000 0000 1000	//myver:xxxx xxxx x101 0100
#define CRITICAL_MASK 0x002B	//PM, Laser, Xbox, camera
#define NONCRITICAL_MASK 0x0054	//GPS, Vehicle Control, Display 
#define NUM_UNITS 6

// |:Unused:HLevel:|:OpenGLView:GPS:UGV:Simulator:|:Remote:Plot:Laser:PM:|
// xxxx xxx0 1000 1011 = 0x009B = Operating
// xxxx xxx0 x000 1011 = 0x000B = Critical Mask 
// xxxx xxx0 x111 0100 = 0x0074 = NonCritical Mask 

//Start up sequence, order in which they will run 

TCHAR Units[10][20] = //Max processes is 10 , array of chars 
{
 TEXT("Laser.exe"),
 TEXT("MTRN3500Ass2.exe"),
 TEXT("VehicleControl.exe"),
 TEXT("RemoteControl.exe"),
 TEXT("GPS.exe"),
 TEXT("Camera.exe"),
};

//Declaration of IsProcessRunning()
bool IsProcessRunning(const char *processName); 
STARTUPINFO s[10];
PROCESS_INFORMATION p[10];
int StartProcess(int i) {
	if (!IsProcessRunning((Units[i]))) //Check if each process is running
	{
		ZeroMemory(&s[i], sizeof(s[i]));
		s[i].cb = sizeof(s[i]);
		ZeroMemory(&p[i], sizeof(p[i]));
		//Start the child processes. 

		if (!CreateProcess(NULL,	//process creation 
			Units[i],				//unit number (each unit is a process)
			NULL,
			NULL,
			FALSE,
			CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			&s[i],
			&p[i])
			)
		{
			printf("%s failed (%d). \n", Units[i], GetLastError()); //if process failed, report that process could not be started
			_getch();
			return -1;
		}
	}
	std::cout << "Started: " << Units[i] << endl; //otherwise everything is started
	Sleep(100);
}

int main()
{	/* Declare some variables */
	__int64 Frequency, HPCCount; 
	double PMTimeStamp; 
	int GPSFailCount = 0; 
	int DisplayFailCount = 0; 
	int VCFailCount = 0; 
	int NonCriticalMaskCount = 0; 
	int CriticalMaskCount = 0; 

	/*Stores program execution properties for each process*/
	//STARTUPINFO s[10]; 
	//PROCESS_INFORMATION p[10]; 

	/*Declare Shared Memory objects for process managent, gps, laser, plotting, simulator, ugv and xbox
	as defined in SMStruct.h */
	TCHAR PMT[] = TEXT("ProcessManagement");
	TCHAR GP[] = TEXT("GPS");
	TCHAR LSR[] = TEXT("Laser"); 
	TCHAR PLOT[] = TEXT("Plotting");
	TCHAR SIM[] = TEXT("Simulator");
	TCHAR UG[] = TEXT("UGV");
	
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	SMObject GPSObj(GP, sizeof(GPS));
	SMObject LaserObj(LSR, sizeof(Laser));
	SMObject PlotObj(PLOT, sizeof(Plotting));
	SMObject SimObj(SIM, sizeof(Simulator));
	SMObject UGVObj(UG, sizeof(UGV));
	TCHAR XB[] = TEXT("XBox");
	SMObject XBoxObj(XB, sizeof(Remote));

	/*Create the actual shared memory space for each SMobject by calling member function */
	PMObj.SMCreate(); 
	GPSObj.SMCreate(); 
	LaserObj.SMCreate(); 
	PlotObj.SMCreate(); 
	SimObj.SMCreate(); 
	UGVObj.SMCreate(); 
	XBoxObj.SMCreate(); 
	/*Get access from this process to shared memory for PM and Xbox (needed for shutdown?) */
	PMObj.SMAccess(); 
	XBoxObj.SMAccess(); 
	PlotObj.SMAccess(); //test if can access from here

	/*Start all other modules listed in the Units array */
	std::cout << "Started: PM.exe" << endl; 
	for (int i = 0; i < NUM_UNITS; i++)	//create processes 1by1 in this loop
	{
		if (!IsProcessRunning((Units[i]))) //Check if each process is running
		{
			ZeroMemory(&s[i], sizeof(s[i]));
			s[i].cb = sizeof(s[i]); 
			ZeroMemory(&p[i], sizeof(p[i]));
			//Start the child processes. 

			if (!CreateProcess(NULL,	//process creation 
				Units[i],				//unit number (each unit is a process)
				NULL,					
				NULL,
				FALSE,
				CREATE_NEW_CONSOLE,
				NULL,
				NULL,
				&s[i],
				&p[i])
				)
			{
				printf("%s failed (%d). \n", Units[i], GetLastError()); //if process failed, report that process could not be started
				_getch(); 
				return -1; 
			}
		}
		std::cout << "Started: " << Units[i] << endl; //otherwise everything is started
		Sleep(100); 
	}

	QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency); //Get HPC Frequency

	/*Create pointers for process management and xbox*/
	ProcessManagement * PM = (ProcessManagement*)PMObj.pData; //Pointer to start of PM shared memory 
	Remote *pXBox = (Remote*)XBoxObj.pData; //Pointer to start of xbox shared memory 
	Plotting *pPlot = (Plotting*)PlotObj.pData; 
	//Initialise shutdown control of all processes
	PM->Shutdown.Status = 0x00;	//process management sets all process to running (no shutdown condition)  

	//START MAIN LOOP
	while (!PM->Shutdown.Flags.PM)	//TODO: while shutdown flag for PM process is not one , inlcude in all modules
	{
		Sleep(50);
		PM->Heartbeat.Flags.PM = 1; //TODO: I am alive , include this in all module process. 
		QueryPerformanceCounter((LARGE_INTEGER*)&HPCCount); //Get HPC Count 
		PMTimeStamp = (double)HPCCount / (double)Frequency;//Calculate TimeStamp
		PM->PMTimeStamp = PMTimeStamp; //Assign timestamp to PM object 
		//std::cout << std::hex << PM->Heartbeat.Status << std::endl; 
	// Check all-process status for critical and non critical conditions
		//std::cout << std::hex << PM->Heartbeat.Status << std::endl; 

		//Non Critical Processes - GPS, Vehicle Control, Display 
		if (PM->Heartbeat.Flags.GPS == 0 )	//GPS
		{
			GPSFailCount++; 
			if (GPSFailCount > 100)
			{
				//Attempt Restart of non-critical processes
				std::cout << "ATTEMPT RESTART NONCRITICAL: GPS" << std::endl; 
				StartProcess(4); 
				GPSFailCount = 0; 
			}
		}
		else
		{
			GPSFailCount = 0;
		}
		if (PM->Heartbeat.Flags.VehicleControl == 0)	//Vehicle Control
		{
			VCFailCount++;
			if (VCFailCount > 100)
			{
				//Attempt Restart of non-critical processes
				std::cout << "ATTEMPT RESTART NONCRITICAL: VEHICLE CONTROL" << std::endl;
				StartProcess(2);
				VCFailCount = 0;
			}
		}
		else
		{
			VCFailCount = 0;
		}
		if (PM->Heartbeat.Flags.Visualisation == 0)	//Display
		{
			DisplayFailCount++;
			std::cout << "DISPLAYFAIL " << DisplayFailCount << std::endl; 
			if (DisplayFailCount > 100)
			{
				//Attempt Restart of non-critical processes
				std::cout << "ATTEMPT RESTART NONCRITICAL: DISPLAY" << std::endl;
				StartProcess(1);
				DisplayFailCount = 0;
			}
		}
		else
		{
			DisplayFailCount = 0;
		}
		

		/*if ((PM->Heartbeat.Status & NONCRITICAL_MASK) != NONCRITICAL_MASK)
		{
			NonCriticalMaskCount++;
			if (NonCriticalMaskCount > 100)
			{
				//TODO: Attempt Restart of non-critical processes
				NonCriticalMaskCount = 0; 
			}
		}
		else
		{
			NonCriticalMaskCount = 0; 
		}*/ 

		//Critical : PM, Laser, Xbox, camera 
		if (PM->Heartbeat.Flags.PM == 0 || PM->Heartbeat.Flags.Laser == 0 || PM->Heartbeat.Flags.Remote == 0 || PM->Heartbeat.Flags.Camera == 0)
		{
			CriticalMaskCount++; 
			//std::cout << "Critical Mask Count " << CriticalMaskCount << std::endl; 
			if (CriticalMaskCount > 2000) {
				std::cout << "CRITICAL FAILURE... SHUTTING DOWN" << std::endl; 
				PM->Shutdown.Status = 0xFFFF; 
				CriticalMaskCount = 0; 
			}
		}
		else
		{
			CriticalMaskCount = 0; 
		}
		/*if (PM->Heartbeat.Status & CRITICAL_MASK != CRITICAL_MASK)//Enter into safe mode if critical failure (could mean shut down)
		{
			CriticalMaskCount++; 
			if (CriticalMaskCount > 100)
			{
				PM->Shutdown.Status = 0xFFFF; //shut down all processes. 
				CriticalMaskCount = 0; 
				break; 
			}
		}
		else
		{
			CriticalMaskCount = 0; 
		} */

	//Trigger shutdown from xbox controller 
		if (pXBox->Terminate)	//TODO: not implemented yet 
		{
			break; 
		}

	//Trigger shutdown from keyboard press
		if (_kbhit())
			break; 
	//Reset heartbeat status, so that heartbeats of each process can be detected in the next iteration
		//Print Heartbeats out 
		std::cout << "Heartbeats - PM: " << PM->Heartbeat.Flags.PM << " Laser:  " << PM->Heartbeat.Flags.Laser << " GPS: " << PM->Heartbeat.Flags.GPS << " Camera: " << PM->Heartbeat.Flags.Camera << " VC: " << PM->Heartbeat.Flags.VehicleControl << " Xbox: " << PM->Heartbeat.Flags.Remote << " Vis: " << PM->Heartbeat.Flags.Visualisation << " Time: " << PM->PMTimeStamp << std::endl;
		//test if can access shared memory of other objects 
		//std::cout << "Visualsiation timestamp: " << pPlot->PlotTimeStamp << std::endl; 
		PM->Heartbeat.Status = 0x00;  
	}

	//at termination (after main loop) 
	//shutdown modules in specified order (this is just an example order) 
	PM->Shutdown.Flags.VehicleControl = 1; //this will break the main while loop of UGV process
	//Sleep(300); //wait 300ms
	PM->Shutdown.Flags.Laser = 1; 
	//Sleep(100); 
	PM->Shutdown.Status = 0xFFFF; //This will break all while loops 
	std::cout << "Terminating normally" << std::endl;
	//release allocated memory
	//automaticaly done by destructor
	//Exit
	return 0; 
}

//Is process running function
bool IsProcessRunning(const char *processName)
{
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry))
		while (Process32Next(snapshot, &entry))
			if (!_stricmp((const char*)(entry.szExeFile), processName))
				exists = true;

	CloseHandle(snapshot);
	return exists;
}
