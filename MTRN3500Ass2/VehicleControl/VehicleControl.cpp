#define _USE_MATH_DEFINES

#using <System.dll>


#include "pch.h"
#include <sstream>
#include <string>
#include <iostream>
#include <Windows.h>
#include <SMObject.h>
#include <SMStruct.h>
#include <math.h>

//Define system namespaces
using namespace System;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::IO;
using namespace System::Text;

ref class UGVClass
{
private:
	//variables for connnecting 
	String ^ HostName; 
	int PortNumber; 
	TcpClient^ Client; 
	NetworkStream^ Stream; 

	//variables for ugv
	bool flag; 
public: 
	UGVClass() {
		flag = 0; 
	};
	~UGVClass() {}; 
	UGVClass(String^ hostName, int portNumber) 
	{
		flag = 0; //initialise private var 

		//Connect & Authenticate with Vehicle 
		HostName = hostName; 
		PortNumber = portNumber; 
		Client = gcnew TcpClient(HostName, PortNumber); 
		// Client settings
		Client->NoDelay = true;
		Client->ReceiveBufferSize = 1024;
		Client->ReceiveTimeout = 500; //ms
		Client->SendBufferSize = 1024;
		Client->SendTimeout = 500;//ms
		Stream = Client->GetStream();	//assign network stream to Stream from tcpclient 
		//authenticate with the server
		// i.e. send student number and check response
		Stream = Client->GetStream();
		String^ ID = gcnew String("5076088\n");
		array<unsigned char>^ WriteID = System::Text::Encoding::ASCII->GetBytes(ID);
		this->Stream->Write(WriteID, 0, WriteID->Length);
		System::Threading::Thread::Sleep(500);
		//Read response
		array<unsigned char>^ ReadData = gcnew array<unsigned char>(16);
		Stream->Read(ReadData, 0, ReadData->Length);
		String^ ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
		//Console::WriteLine(ResponseData); 
	}

	void Drive(double steer, double speed)	//function that sends steer and speed command to the vehicle 
	{
		
		//do we need to use stx and etx? 
		char Buffer[128];		//create char buffer
		//how to initialise to 0 if the controller isnt inputting anything, perhaps initialise in xbox instead
		flag = 1 - flag;	//alternate flag
		sprintf_s(Buffer, "# %f %f %d #", steer, speed, flag);	//create command string 
		//SendData(Buffer, strlen(Buffer)); // send command string 
		
		//PRINT THE COMMAND THAT IS BEING SENT TO THE VEHICLE 
		std::cout << Buffer << std::endl; 

		String^ RequestScan = gcnew String(Buffer);	//send LMS command request (single measured value ooutput)
		//String^ RequestScan = gcnew String("sMN Run");	//alternative string command, refer to manual 
		array<unsigned char>^ WriteBuf = gcnew array<unsigned char>(128); //create write buffer 
		Int32 bytes;	//create bytes variable
		WriteBuf = System::Text::Encoding::ASCII->GetBytes(RequestScan); //Convert the request to bytes 
		Stream->WriteByte(0x02); //Ascii for STX, start of text
		Stream->Write(WriteBuf, 0, WriteBuf->Length);	//write data to network stream (buffertowrite, offset(wheretostartsend), sizeofdatatowrite)
		Stream->WriteByte(0x03); //Ascii for ETX, end of text 
		System::Threading::Thread::Sleep(50); //delay for 10ms (wait for server to prepare the data) 
		//std::cout << "pre-steps complete" << std::endl;
	}
};
int main()
{

	UGVClass^ MyUGV = gcnew UGVClass("192.168.1.200", 25000); // make ugv object 

	/*Declare Shared Memory objects for process managent, gps
	as defined in SMStruct.h */
	TCHAR PMT[] = TEXT("ProcessManagement");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	TCHAR XB[] = TEXT("XBox");	//declare object
	SMObject XBoxObj(XB, sizeof(Remote));

	/*Create the actual shared memory space for each SMobject by calling member function */
	//PMObj.SMCreate();

	/*Get access from this process to shared memory for PM and Xbox (needed for shutdown?) */
	PMObj.SMAccess();
	XBoxObj.SMAccess();

	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	Remote * pXbox = (Remote*)XBoxObj.pData;


	int PMFailCount = 0; 
	while (!PM->Shutdown.Flags.VehicleControl)
	{
		if (!PM->Heartbeat.Flags.PM)	//check process management is still running 
		{
			PMFailCount++;
			if (PMFailCount >200)
			{
				PMFailCount = 0; 
				break;
			}
		}
		else PMFailCount = 0;
		PM->Heartbeat.Flags.VehicleControl = 1;
		MyUGV->Drive(pXbox->setSteering, pXbox->setSpeed);
		//std::cout << "Speed: " << pXbox->setSpeed << " Steering: " << pXbox->setSteering << std::endl;
	}
}

