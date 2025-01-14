//Compile in a C++ CLR empty project
#using <System.dll>
#define CRC32_POLYNOMIAL			0xEDB88320L
#include <conio.h>//_kbhit()
#include "pch.h"
#include <sstream>
#include <string>
#include <iostream>
#include <Windows.h>
#include <SMObject.h>
#include <SMStruct.h>
#include <math.h>

using namespace System;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

#pragma pack(1)
struct GPSData
{
	char header[3]; 
	unsigned char headerlength; 
	unsigned short messageID; 
	char messageType;
	unsigned char port; 
	unsigned short msglength; 
	unsigned short sequence; 
	unsigned char idle; 
	unsigned char timesatus;
	unsigned short gpsweek; 
	char gpsms[4]; 
	unsigned long receiverstamp; 
	unsigned short reserved1; 
	unsigned short receiver; 

	char status[4];
	char postype[4]; 
	char longitudinal[4]; 
	char latitudinal[4]; 
	double northing; 
	double easting; 
	double height; 
	float undulation; 
	char datum[4]; 
	float nsd; 
	float esd; 
	float hsd; 
	char basestation[4]; 
	float diffage; 
	float solage;
	unsigned char satnum; 
	unsigned char gpsl1; 
	unsigned char gpsl1rtk; 
	unsigned char gpsl2rtk; 
	unsigned char reserved[4]; 
	unsigned long crc; 
};

union GPSData1
{
	GPSData data; 
	unsigned char dataArray[112];
};

unsigned long CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}
/* --------------------------------------------------------------------------
Calculates the CRC-32 of a block of data all at once
-------------------------------------------------------------------------- */
unsigned long CalculateBlockCRC32(unsigned long ulCount, /* Number of bytes in the data block */
	unsigned char *ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}

//Note: Data Block ucBuffer should contain al




int main()
{
	double GPSTimeStamp;
	__int64 Frequency, HPCCount;
	QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency); //Get HPC Frequency
	//Declare Shared Memory objects for process managent, gps
	//as defined in SMStruct.h
	TCHAR PMT[] = TEXT("ProcessManagement");
	TCHAR GP[] = TEXT("GPS");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	SMObject GPSObj(GP, sizeof(GPS));

	//Create the actual shared memory space for each SMobject by calling member function 
	//PMObj.SMCreate();
	//GPSObj.SMCreate();

	//Get access from this process to shared memory for PM and Xbox (needed for shutdown?) 
	PMObj.SMAccess();
	GPSObj.SMAccess();

	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	GPS *pGPS = (GPS*)GPSObj.pData;

	// Galil PLC any port number is OK
	int PortNumber = 24000;
	// Pointer to TcpClent type object on managed heap
	TcpClient^ Client;
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData; 
	array<unsigned char>^ ReadData;
	// String command to ask for Channel 1 analogue voltage from the PLC
	// These command are available on Galil RIO47122 command reference manual
	// available online
	String^ AskVoltage = gcnew String("MG @AN[0];");
	// String to store received data for display
	String^ ResponseData;
	unsigned int Header = 0 ; 

	// Creat TcpClient object and connect to it
	Client = gcnew TcpClient("192.168.1.200", PortNumber);
	// Configure connection
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;//ms
	Client->SendTimeout = 500;//ms
	Client->ReceiveBufferSize = 1024; 
	Client->SendBufferSize = 1024;  

	// unsigned char arrays of 16 bytes each are created on managed heap
	SendData = gcnew array<unsigned char>(16);
	ReadData = gcnew array<unsigned char>(150);
	// Convert string command to an array of unsigned char
	SendData = System::Text::Encoding::ASCII->GetBytes(AskVoltage);

	// Get the network stream object associated with client so we 
	// can use it to read and write
	NetworkStream^ Stream = Client->GetStream();

	bool reading = 0; //signal that data is being read in (vs waiting for header detect) 
	int PMFailCount = 0; 
	//Loop
	while (!PM->Shutdown.Flags.GPS)
	{
		/*if (!PM->Heartbeat.Flags.PM)	//check process management is still running 
		{
			PMFailCount++;
			if (PMFailCount > 200)
			{
				PMFailCount = 0; 
				break;
			}
		}
		else PMFailCount = 0; */ 
		QueryPerformanceCounter((LARGE_INTEGER*)&HPCCount); //Get HPC Count 
		GPSTimeStamp = (double)HPCCount / (double)Frequency;//Calculate TimeStamp
		PM->Heartbeat.Flags.GPS = 1;
		if (Stream->DataAvailable) {
			// Write command asking for data
			Stream->Write(SendData, 0, SendData->Length);
			// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
			System::Threading::Thread::Sleep(10);
			// Read the incoming data
			Stream->Read(ReadData, 0, ReadData->Length);
			// Print the received string on the screen
			for (int i = 0; i < 150; i++) {
				//printf("%x", ReadData[i]);
				if (i < 50 && ReadData[i] == 0xaa && ReadData[i + 1] == 0x44 && ReadData[i + 2] == 0x12)
				{
					GPSData1 DataGPS;
					for (int j = 0; j < 112; j++)
					{
						DataGPS.dataArray[j] = ReadData[j]; 
					}
					
					unsigned long crcCheck = CalculateBlockCRC32((unsigned long)108, /* Number of bytes in the data block */
						(unsigned char*)DataGPS.dataArray); /* Data block */
					
					//Assign data to shared memory 
					if (DataGPS.data.crc == crcCheck)
					{
						pGPS->Easting = DataGPS.data.easting; 
						pGPS->Northing = DataGPS.data.northing; 
						pGPS->height = DataGPS.data.height; 
						pGPS->GPSTimeStamp = GPSTimeStamp; 
						std::cout << "Time: " << pGPS->GPSTimeStamp; 
						std::cout << "\tn:  " << DataGPS.data.northing;
						std::cout << "\te:  " << DataGPS.data.easting;
						std::cout << "\th: " << DataGPS.data.height;
						std::cout << "\tcrc: " << DataGPS.data.crc;
						std::cout << "\tcrcCheck: " << crcCheck << std::endl;
					}
					//reading = 1; 
					int headerLength = ReadData[i + 3]; 
					//std::cout << std::endl; 
					//std::cout << "headerlength = " << headerLength << std::endl; 
					
					//MSB first 1:2:...:8 
					char North[8]; 
					North[0] = ReadData[i + headerLength + 16];
					North[1] = ReadData[i + headerLength + 17];
					North[2] = ReadData[i + headerLength + 18];
					North[3] = ReadData[i + headerLength + 19];
					North[4] = ReadData[i + headerLength + 20];
					North[5] = ReadData[i + headerLength + 21];
					North[6] = ReadData[i + headerLength + 22];
					North[7] = ReadData[i + headerLength + 23];
					//("Northing : "); 
					//for (int i = 0; i < 8; i++) {
						//printf("%x", North[i]); 
					//}
					std::string NorthString = North; 
					//std::cout << NorthString; 
					//printf("%x", NorthString); 
					//printf("\n"); 
					
					char East[8];
					East[0] = ReadData[i + headerLength + 24];
					East[1] = ReadData[i + headerLength + 25];
					East[2] = ReadData[i + headerLength + 26];
					East[3] = ReadData[i + headerLength + 27];
					East[4] = ReadData[i + headerLength + 28];
					East[5] = ReadData[i + headerLength + 29];
					East[6] = ReadData[i + headerLength + 30];
					East[7] = ReadData[i + headerLength + 31];
					//printf("Easting: ");
					//for (int i = 0; i < 8; i++) {
					//	printf("%x", East[i]);
					//}
					std::string EastString = East; 
					//printf("%x", EastString); 
					//std::cout << EastString; 
					//printf("\n");

					//convert string of hex values to int
					std::stringstream is(NorthString); 
					double northTing; 
					is >> std::hex >> northTing; 
					//std::cout << "northTing " << northTing << std::endl;

					std::stringstream is1(EastString); 
					double eastTing; 
					is1 >> std::hex >> eastTing;
					//std::cout << "eastTing " << eastTing << std::endl;
					 
					char Height[8]; 
					for (int j = 7; j >=0; j--)
					{
						Height[j] = ReadData[i + headerLength + 32 + j];
					}
					//printf("Height: "); std::string HeightString = Height; 
					//printf("%x", HeightString); printf("\n"); 


					//std::cout << std::hex <<  "NOrthing = " << North2 << North3 << North4 << North5 << North6 << North7 << North8 << std::endl; 
					//std::cout << "northing = " << North << std::endl;
				}
			}
		}
	}
	Stream->Close();
	Client->Close();
	//Console::ReadKey();
	return 0;
}


/*
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

//Define struct for x,y point 
value struct Point
{
	double X;
	double Y;
};

//Define laser class
ref class GPSClass
{
private:
	String ^ HostName;	//stores host name, port number, start angle, resolution, client and stream 
	int PortNumber;
	int NumData;	//number of data?  
	double StartAngle; //starting angle of laser scanner
	double Resolution;	//resolution of laser 
	TcpClient^ Client;	//client to communicate with 
	NetworkStream^ Stream; //stream of data 
public:
	array<unsigned char>^ Data = gcnew array<unsigned char>(2048);	//data from the laser scanner
	array<Point>^ Ranges;	//Range data 
public:
	GPSClass() {}
	~GPSClass() {};
	GPSClass(String^ hostName, int portNumber)	//define overload constructor
	{
		HostName = hostName;
		PortNumber = portNumber;
		Client = gcnew TcpClient(HostName, PortNumber);	//set up a tcp client with the given host and port

		// Client settings
		Client->NoDelay = true;
		Client->ReceiveBufferSize = 1024;
		Client->ReceiveTimeout = 500; //ms
		Client->SendBufferSize = 1024;
		Client->SendTimeout = 500;//ms
		Stream = Client->GetStream();	//assign network stream to Stream from tcpclient 
		//authenticate with the server
		// i.e. send student number and check response
		//Stream = Client->GetStream();
		//String^ ID = gcnew String("5076088\n");
		//array<unsigned char>^ WriteID = System::Text::Encoding::ASCII->GetBytes(ID);
		//this->Stream->Write(WriteID, 0, WriteID->Length);
		//System::Threading::Thread::Sleep(10);

		//Read response
		//array<unsigned char>^ ReadData = gcnew array<unsigned char>(16);
		//Stream->Read(ReadData, 0, ReadData->Length);
		//String^ ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
		//Console::WriteLine(ResponseData);
	}

	array<Point>^ GetSingleScan()	//function to get a single scan , returns array pointer of type point
	{
		char RxData[2084];
		std::cout << "Single scan called" << std::endl;
		//Send request, then receive reply 
		String^ RequestScan = gcnew String("");	//send LMS command request (single measured value ooutput)
		//String^ RequestScan = gcnew String("sMN Run");	//alternative string command, refer to manual 
		array<unsigned char>^ WriteBuf = gcnew array<unsigned char>(128); //create write buffer 
		Int32 bytes;	//create bytes variable
		WriteBuf = System::Text::Encoding::ASCII->GetBytes(RequestScan); //Convert the request to bytes 
		Stream->WriteByte(0x02); //Ascii for STX, start of text
		Stream->Write(WriteBuf, 0, WriteBuf->Length);	//write data to network stream (buffertowrite, offset(wheretostartsend), sizeofdatatowrite)
		Stream->WriteByte(0x03); //Ascii for ETX, end of text 
		System::Threading::Thread::Sleep(10); //delay for 10ms (wait for server to prepare the data) 
		std::cout << "pre-steps complete" << std::endl;
		if (Client->Available != 0)	//read data 
		{
			std::cout << "client available" << std::endl;
			bytes = Stream->Read(Data, 0, Data->Length); //get laser data from stream, assign to data
			//Convert the data to string stream and finding points
			for (int i = 0; i < 2048; i++)
			{
				RxData[i] = Data[i];
			}
			std::string ScanResp = RxData;
			std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
			//filter out the irrelevant data
			std::string Fragments;
			for (int i = 0; i < 25; i++)
			{
				is >> Fragments;
			}
			//is >> hex >> Numpoints; //numpoints = numdata
			//for (int i = 0; i < NumPoints; i++)
			//{
			//	is >> std::hex >> Range[i];
			//}
			//end of conversion to stringstream
			String^ dataString = System::Text::Encoding::ASCII->GetString(this->Data);//convert data to string^ 
			Console::WriteLine(dataString);
		}
		if (bytes == 0)
		{
			std::cout << "client not available " << std::endl;
			return nullptr;
		}
		else
		{
			GetResolution();	//from the bytes data, get these 
			GetStartAngle();
			GetNumData();
			UpdateRanges();
			return Ranges;
		}
	}
private:
	void GetResolution()
	{
		//TODO: get resolution 
		Resolution = 0;//process Data to get resolution, i.e. skip 24 spaces (0x20) in Data,
					   //then fill in an Int16 byte by byte, divide result by 10000.

		// one way to do this is to copy data into a stringstream object
		// then read from it into a string object until you get to the data you want
		// then read in the resolution (its stored a hex value)

	}

	void GetStartAngle()
	{
		StartAngle = 45.0; // process Data to get StartAngle (skip 23 spaces)
						   // then fill in an int32 byte by byte and divide the result by 10000.

		// do similar to above
	}

	void GetNumData()
	{
		NumData = 0; // procrss Data to get NumData skip 25 spaces and
					 // then fill in an uint_16  byte by byte.

		// do similar to above
	}

	void UpdateRanges()
	{
		Ranges = gcnew array<Point>(NumData);
		// process Data to update ranges skip 26 spaces, then run a for loop
		for (int i = 0; i < NumData; i++)
		{
			Ranges[i].X = 0;// Process data to fill these values
			Ranges[i].Y = 0;// Process data to fill these values
		}
	}

};

int main()
{
	std::cout << " start gps" << std::endl;
	GPSClass^ MyGPS = gcnew GPSClass("192.168.1.200", 24000);

	std::cout << "gps started " << std::endl;
	//Declare Shared Memory objects for process managent, gps
	as defined in SMStruct.h 
	TCHAR PMT[] = TEXT("ProcessManagement");
	TCHAR GP[] = TEXT("GPS");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); //pass szName and size 
	SMObject GPSObj(GP, sizeof(GPS));

	//Create the actual shared memory space for each SMobject by calling member function 
	//PMObj.SMCreate();
	//GPSObj.SMCreate();

	//Get access from this process to shared memory for PM and Xbox (needed for shutdown?) 
	PMObj.SMAccess();
	GPSObj.SMAccess(); 

	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	GPS *pGPS = (GPS*)GPSObj.pData;

	while (!PM->Shutdown.Flags.GPS)
	{
		PM->Heartbeat.Flags.GPS = 1;
		//std::cout << "GPS is running" << std::endl;
	}
}


*/

