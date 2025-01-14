#pragma comment(lib, "XInput.lib")


#include <Windows.h>
#include <xinput.h>
#include <sstream>
#include <iostream>
#include <Windows.h>
#include <SMObject.h>
#include <SMStruct.h>


using namespace std;

int main()
{
	bool connectflag = false; 
	//xbox stuff
	XINPUT_BATTERY_INFORMATION BatteryInformation;
	XINPUT_STATE State;
	XINPUT_VIBRATION Vibration;
	//XInputEnable(true);

	// display battery level 
	XInputGetBatteryInformation(0, BATTERY_DEVTYPE_GAMEPAD, &BatteryInformation);
	switch (BatteryInformation.BatteryLevel)
	{
	case BATTERY_LEVEL_EMPTY: cout << "Battery level empty " << endl;
		break;
	case BATTERY_LEVEL_LOW: cout << "Battery level low " << endl;
		break;
	case BATTERY_LEVEL_MEDIUM: cout << "Battery level medium " << endl;
		break;
	case BATTERY_LEVEL_FULL: cout << "Battery level full" << endl;
	}
	Sleep(3000);


	//Declare Shared Memory objects for process managent, gps
	//as defined in SMStruct.h 
	TCHAR PMT[] = TEXT("ProcessManagement");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	TCHAR XB[] = TEXT("XBox");	//declare object
	SMObject XBoxObj(XB, sizeof(Remote));

	//Create the actual shared memory space for each SMobject by calling member function 
	//PMObj.SMCreate();

	//Get access from this process to shared memory for PM and Xbox (needed for shutdown?) 
	PMObj.SMAccess();
	XBoxObj.SMAccess(); 

	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	Remote * pXbox = (Remote*)XBoxObj.pData; 

	int PMFailCount = 0; 
	int ControllerNum = 0; 

	//test if xbox connected
	for (int i = 0; i < 4; i++) {
		if (XInputGetState(i, &State) == ERROR_SUCCESS) {
			cout << i << " is connected" << endl;
			ControllerNum = i; 
			connectflag = true; 
		}
		else if (XInputGetState(i, &State) == ERROR_DEVICE_NOT_CONNECTED) {
			cout << i << " is not connected" << endl;
			pXbox->setSpeed = 0; 
			pXbox->setSteering = 0; 
			connectflag = false; 
		}
	}

	while (!PM->Shutdown.Flags.Remote)
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
		PM->Heartbeat.Flags.Remote = 1;
		//std::cout << "Remote is running" << std::endl;

		//test xbox still connected
		if (XInputGetState(ControllerNum, &State) == ERROR_SUCCESS) {
			connectflag = true; 
		}
		else if (XInputGetState(ControllerNum, &State) == ERROR_DEVICE_NOT_CONNECTED) {
			cout << "Controller not connected" << endl;
			pXbox->setSpeed = 0; 
			pXbox->setSteering = 0; 
			connectflag = false; 
		}



		
		// Add code to get xbox input and assign to shared memory here
		
		if (connectflag)
		{
			if (State.Gamepad.wButtons == 0x4000)	//x key  
				pXbox->Terminate = 1;

			double SteerVal = 0;  double SpeedVal = 0;
			if (abs(State.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				SteerVal = 4 * (State.Gamepad.sThumbRX / 3276);
			}
			if (abs(State.Gamepad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
				SpeedVal = (State.Gamepad.sThumbLY / 3276) / 10.0;
			}
			pXbox->setSpeed = SpeedVal; pXbox->setSteering = SteerVal;
			std::cout << "Speed: " << pXbox->setSpeed << " Steering: " << pXbox->setSteering << std::endl;
		}
	}
}

