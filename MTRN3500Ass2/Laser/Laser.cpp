#define _USE_MATH_DEFINES

//Either include metadata from System.dll by including it here or by going to project properties
// and then C++->Advanced->Forced #using file
#using <System.dll>

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
	int N; 
};

//Define laser class
ref class LaserClass
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
	LaserClass() {}
	~LaserClass() {};
	LaserClass(String^ hostName, int portNumber)	//define overload constructor
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
		Stream = Client->GetStream(); 
		String^ ID = gcnew String("5076088\n");
		array<unsigned char>^ WriteID = System::Text::Encoding::ASCII->GetBytes(ID);
		this->Stream->Write(WriteID, 0, WriteID->Length); 
		System::Threading::Thread::Sleep(10);

		//Read response
		array<unsigned char>^ ReadData = gcnew array<unsigned char>(16);
		Stream->Read(ReadData, 0, ReadData->Length); 
		String^ ResponseData = System::Text::Encoding::ASCII->GetString(ReadData); 
		//Console::WriteLine(ResponseData); 
	}

	array<Point>^ GetSingleScan()	//function to get a single scan , returns array pointer of type point
	{
		//std::cout << "Single scan called" << std::endl; 
		//Send request, then receive reply 
		String^ RequestScan = gcnew String("sRN LMDscandata");	//send LMS command request (single measured value ooutput)
		//String^ RequestScan = gcnew String("sMN Run");	//alternative string command, refer to manual 
		array<unsigned char>^ WriteBuf = gcnew array<unsigned char>(128); //create write buffer 
		Int32 bytes;	//create bytes variable
		WriteBuf = System::Text::Encoding::ASCII->GetBytes(RequestScan); //Convert the request to bytes 
		Stream->WriteByte(0x02); //Ascii for STX, start of text
		Stream->Write(WriteBuf, 0, WriteBuf->Length);	//write data to network stream (buffertowrite, offset(wheretostartsend), sizeofdatatowrite)
		Stream->WriteByte(0x03); //Ascii for ETX, end of text 
		System::Threading::Thread::Sleep(10); //delay for 10ms (wait for server to prepare the data) 
		//std::cout << "pre-steps complete" << std::endl;
		if (Client->Available != 0)	//read data 
		{
			//std::cout << "client available" << std::endl;
			bytes = Stream->Read(Data, 0, Data->Length); //get laser data from stream, assign to data
			//Convert the data to string stream and finding points
			char RxData[2084];
			for (int i = 0; i < 2048; i++)
				RxData[i] = Data[i];		//copy array char^ into standard char array 
			std::string ScanResp = RxData;	//assign the char array to a string variable 
			std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
			//std::cout << ScanResp << std::endl; 
			//CHECK IF DATA IS VALID BY CHECKING THAT FIRST WORD IS STX
			char STX = '\u0002';
			char firstWord;
			is >> firstWord; 
			//std::cout << "firstword = " <<firstWord << std::endl; 
			if (firstWord != STX) {
				bytes = 0; 
				//std::cout << "stx not detected " << std::endl;
			}
			
			//check if firstword = <STX> ; 
			//for (int i = 0; i < 25; i++)
			//{
			//	is >> Fragments; 
			//}
			//is >> hex >> Numpoints; //numpoints = numdata
			//for (int i = 0; i < NumPoints; i++)
			//{
			//	is >> std::hex >> Range[i];
			//}
			//end of conversion to stringstream
			//String^ dataString = System::Text::Encoding::ASCII->GetString(this->Data);//convert data to string^ 
			//Console::WriteLine(dataString); 
		}
		if (bytes == 0)
		{
			//std::cout << "client not available " << std::endl;
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
		//Convert the data to string stream object 
		char RxData[2084];
		for (int i = 0; i < 2048; i++)
		{
			RxData[i] = Data[i];		//copy array char^ into standard char array 
		}
		std::string ScanResp = RxData;	//assign the char array to a string variable 
		std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
		std::string word;
		int count = 0;
		//while loop to sort through data 
		while (is >> word)
		{
			std::istringstream s1(word); 
			//GET RESOLUTION 
			if (count == 24)
			{
				int reso = 0;
				s1 >> std::hex >> reso;  //to get decimal version into reso do s1 >> std::hex >> reso; 
				//std::cout << "Resolution: " << reso << std::endl;
				Resolution = reso / 10000.0; //gets the resolution in deg
				std::cout << "Resolution: " << Resolution << " ";
			}
			count++; 
		}
	}

	void GetStartAngle()
	{
		//Convert the data to string stream object 
		char RxData[2084];
		for (int i = 0; i < 2048; i++)
		{
			RxData[i] = Data[i];		//copy array char^ into standard char array 
		}
		std::string ScanResp = RxData;	//assign the char array to a string variable 
		std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
		std::string word;
		int count = 0;
		//while loop to sort through data 
		while (is >> word)
		{
			std::istringstream s1(word);
			//GET START ANGLE 
			if (count == 23)
			{
				int startAng = 0;
				s1 >> std::hex >> startAng;
				//std::cout << "Start Angle: " << startAng << std::endl;
				double StartAngle = startAng / 10000.0; //gets the start angle  deg
				std::cout << "Start Angle: " << StartAngle << " ";
			}
			count++;
		}
	}

	void GetNumData()
	{
		//Convert the data to string stream object 
		char RxData[2084];
		for (int i = 0; i < 2048; i++)
		{
			RxData[i] = Data[i];		//copy array char^ into standard char array 
		}
		std::string ScanResp = RxData;	//assign the char array to a string variable 
		std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
		std::string word;
		int count = 0;
		//while loop to sort through data 
		while (is >> word)
		{
			int numofData = 0;//declare numofData var in scope of while loop 
			std::istringstream s1(word);
			if (count == 25)
			{
				s1 >> std::hex >> numofData;
				NumData = numofData; 
				std::cout << "NumData: " << NumData;
			}
			count++;
		}
		
	}

	void UpdateRanges()
	{
		Ranges = gcnew array<Point>(NumData);
		unsigned int Range; 
		//Convert the data to string stream object 
		char RxData[2084];
		for (int i = 0; i < 2048; i++)
		{
			RxData[i] = Data[i];		//copy array char^ into standard char array 
		}
		std::string ScanResp = RxData;	//assign the char array to a string variable 
		std::istringstream is(ScanResp); //ifstream is("InputFile.txt"); 
		// process Data to update ranges skip 26 spaces, then run a for loop
		std::string Fragments; 
		for (int i = 0; i < 26; i++)
			is  >> Fragments;
		//std::cout << Fragments << std::endl; //the last fragment before range data should be numdata 
		for (int i = 0; i < NumData; i++)
		{
			is >> std::hex >> Range;
			//std::cout << "Range: " << Range << " "; 
			Ranges[i].X = Range*cos(StartAngle*M_PI / 180.0);
			Ranges[i].Y = Range*sin(StartAngle*M_PI / 180.0);
			Ranges[i].N = NumData; 
			//TODO: Store these range data into shared memory 
			//if (i == 1)
			std::cout << i << ": " << " X: " << Ranges[i].X << " Y: " << Ranges[i].Y << std::endl; 
			StartAngle = StartAngle + Resolution;
			//std::cout << Resolution << std::endl; 
		}
	}
};


int main()
{
	double LaserTimeStamp;
	__int64 Frequency, HPCCount;
	double X[361], Y[361];
	std::cout << " start laser" << std::endl; 
	
	LaserClass^ MyLaser = gcnew LaserClass("192.168.1.200", 23000);	//construcotr takes ip address and port
	
	
	std::cout << "laser started " << std::endl;

	QueryPerformanceFrequency((LARGE_INTEGER *)&Frequency); //Get HPC Frequency

	//Declare Shared Memory Objects for process management and laser data and give access, and declare pointers
	TCHAR PMT[] = TEXT("ProcessManagement"); TCHAR LSR[] = TEXT("Laser");
	SMObject PMObj(PMT, sizeof(ProcessManagement)); SMObject LaserObj(LSR, sizeof(Laser));
	PMObj.SMAccess(); LaserObj.SMAccess();
	ProcessManagement * PM = (ProcessManagement*)PMObj.pData;
	Laser *pLaser = (Laser*)LaserObj.pData;

	array<Point>^ ReceivedRanges; 
	int PMFailCount = 0; 
	//START MAIN LOOP 
	while (!PM->Shutdown.Flags.Laser)	//while no shutdown flag for laser , continue operatioin 
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&HPCCount); //Get HPC Count 
		LaserTimeStamp = (double)HPCCount / (double)Frequency;//Calculate TimeStamp
		
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
		//std::cout << "PMTimeStamp: " << PMTimeStamp << std::endl; 
		PM->Heartbeat.Flags.Laser = 1; //send heartbeat to PM 
		//std::cout << "Laser is Running" << std::endl; 
		ReceivedRanges = MyLaser->GetSingleScan(); //if valid data is received, push it to shared memory
		if (ReceivedRanges != nullptr) {
			int N = ReceivedRanges[1].N; 
			pLaser->NumPoints = N; 
			for (int i = 0; i < N; i++) {
				pLaser->x[i] = ReceivedRanges[i].X; 
				pLaser->y[i] = ReceivedRanges[i].Y;
				//std::cout << "x: " << pLaser->x[i]; 
				//std::cout << " y: " << pLaser->y[i] << std::endl; 
			}
			pLaser->LaserTimeStamp = LaserTimeStamp; //Assign timestamp to LSR object 
			//std::cout << " timestamp: " << pLaser->LaserTimeStamp;
			//std::cout << " x1: " << pLaser->x[1] << " y1: " << pLaser->y[1] << std::endl; // print out point 1 . 
		}
		//Sleep(500); 
	}
	//TODO: continually get laser scan information 
	return 0;
}

