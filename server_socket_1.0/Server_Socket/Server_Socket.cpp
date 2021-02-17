#include <stdio.h>
#include <iostream>
#include <list>
#include <winsock2.h>
#include <chrono>
#include <thread>
#include <string>
#include <windows.h>
#include <iomanip>
#include "struct_messung.h"
#include "print_deffinitions.h"

using namespace std;

#define SERVER_PORT 1234
#define MAX_BUFF_LEN 256

void printStruct(messung* mess);

int main()
{

	system("color 0");
	// var for return values of socketfunctions
	long rc;

	//receive buffer
	char buff[MAX_BUFF_LEN];

	//important strings for communication with arduino see example communication
	char msg_get[] = "GET_SEN";
	char msg_ok[] = "OK";

	int trenn_num = 0;

	messung *current;
	list<messung*> messungen;


	std::chrono::milliseconds dura(2000);


	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 1);
	rc = WSAStartup(wVersionRequested, &wsaData);

	WSAGetLastError();

	//declare work and server/listen socket
	SOCKET listen_socket;//
	SOCKET work_socket;//
	

	//create an tcp socket
	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//check if it returned invalid sockets
	if (listen_socket == INVALID_SOCKET)
	{
		cout << "FEHLER: Erzeugen des Server-Sockets nicht moeglich!" << endl;
	}

	WSAGetLastError();

	//Binden des Serversockets an einen PORT (s.define in der StdAfx.h)
	SOCKADDR_IN adresse;//

	//size of the SOCKADDR_IN structure
	int adresslaenge = sizeof(adresse);
	
	//define protocoll family AF_INET = IPv4
	adresse.sin_family = AF_INET;

	//defines port as SERVER_PORT from define
	adresse.sin_port = htons(SERVER_PORT);

	//i dont the fuck know what this means
	adresse.sin_addr.s_addr = 0;


	//bind server socket on the listen port with error check 
	rc = bind(listen_socket, (SOCKADDR*)&adresse, adresslaenge);
	if (rc == SOCKET_ERROR)
	{
		cout << "FEHLER: Server-Sockets konnte kein PORT zugewiesen werden!" << endl;
	}
	WSAGetLastError();

	//listen for clients with error check
	rc = listen(listen_socket, SOMAXCONN);
	if (rc == SOCKET_ERROR)
	{
		cout << "FEHLER: Kann Anfragen von Clients nicht entgegennehmen!" << endl;
	}
	WSAGetLastError();

	system("color 4");

	//Schleife für Anfragen
	printf(R"(
 _____                             _   _             
/  __ \                           | | (_)            
| /  \/ ___  _ __  _ __   ___  ___| |_ _ _ __   __ _ 
| |    / _ \| '_ \| '_ \ / _ \/ __| __| | '_ \ / _` |
| \__/\ (_) | | | | | | |  __/ (__| |_| | | | | (_| |
 \____/\___/|_| |_|_| |_|\___|\___|\__|_|_| |_|\__, |
                                                __/ |
                                               |___/ 
	)");

	work_socket = NULL;

	while (work_socket == INVALID_SOCKET);
	{
		work_socket = accept(listen_socket, (SOCKADDR*)&adresse, &adresslaenge);
	}
	system("cls");
	system("color 2");
	printf(R"(
  ____                             _           _ 
/  __ \                           | |         | |
| /  \/ ___  _ __  _ __   ___  ___| |_ ___  __| |
| |    / _ \| '_ \| '_ \ / _ \/ __| __/ _ \/ _` |
| \__/\ (_) | | | | | | |  __/ (__| ||  __/ (_| |
 \____/\___/|_| |_|_| |_|\___|\___|\__\___|\__,_|
                                                 
)");

	std::this_thread::sleep_for(dura);

	//--------------------------------------------------------------------------------------------------
	//communication
	

	messung* temp_mess;

	float temp_lux;

	while (1) 
	{
		temp_mess = new messung();

		rc = send(work_socket, msg_get, strlen(msg_get), NULL);

		for (int i = 0; i < 11; i++)
		{

			rc = recv(work_socket, buff, MAX_BUFF_LEN, NULL);

			//checks if recv doesn't give back Error or received mesage is bigger than buffer
			if (rc < 0 || rc > 255)
			{
				cout << "ERROR while reading Data" << endl;
				buff[0] = '0';
				rc = 1;
				break;
			}

			buff[rc] = '\0';

			//checks if arduino sends Back an ERROR
			if (buff == "ERR") 
			{
				cout << "ERROR on client side" << endl;
				break;
			}

			//handles where to save the incomming value in the structure 
			switch (i)
			{
			case 0:
				temp_mess->dat.year = stoi(buff);
				break;
			case 1:
				temp_mess->dat.month = stoi(buff);
				break;
			case 2:
				temp_mess->dat.day = stoi(buff);
				break;
			case 3:
				temp_mess->dat.hour = stoi(buff);
				break;
			case 4:
				temp_mess->dat.minute = stoi(buff);
				break;
			case 5:
				temp_mess->dat.second = stoi(buff);
				break;
			case 6:
				temp_lux = stof(buff);
				temp_lux = temp_lux * 5.0f / 1024.0f; //wandle analogen wert in anliegende Spannung um
				temp_lux = temp_lux / 10000.0f; //wandle anliegende Spannung über intern verbauten wiederstand in ampere um
				temp_lux = temp_lux * 1000000; // wandle in milli Ampere um 
				temp_lux = temp_lux * 2; // wandle milli Ampere anhand vom Datasheet in einheit lux um y=ampere x=lumen Formel -> y=1/2(x)
				temp_mess->lux = temp_lux;
				break;
			case 7:
				temp_mess->dht_1.temp = stof(buff, NULL);
				break;
			case 8:
				temp_mess->dht_1.hum = stof(buff, NULL);
				break;
			case 9:
				temp_mess->dht_2.temp = stof(buff, NULL);
				break;
			case 10:
				temp_mess->dht_2.hum = stof(buff, NULL);
				break;
			}
			//Sends ok back to Arduino
			rc = send(work_socket, msg_ok, strlen(msg_ok), NULL);
		}
		//prints structure in formatted table
		printStruct(temp_mess);

		//add temp_mess pointer to list messungen
		//messungen.push_front(temp_mess);
	}

	//close the open sockets
	closesocket(work_socket);
	closesocket(listen_socket);

	//give back socket resources
	WSACleanup();
	system("pause");
	return 0;
}

// print n times c in current line
void printChar(int n, char c)
{
	for (int i = 0; i < n; i++)
	{
		cout << c;
	}
}

//print table specific Seperator
void printSeperator(char c)
{
	cout << T_r;
	printChar(18, pipe_h);
	cout << c;
	printChar(19, pipe_h);
	cout << T_l << endl;
}


//print stucture values on screen in table
//use of setw() to set width of output stream -> constant table width
//used chars are defined in print_deffinitions.h with decimal 
//only prints messung structure
void printStruct(messung *mess)
{
	system("cls");
	string temp = "";
	cout << edge_u_l;
	printChar(38, pipe_h);
	cout << edge_u_r << endl;
	temp += to_string(mess->dat.day) + "-" + to_string(mess->dat.month) + "-" + to_string(mess->dat.year);
	cout << pipe_v << setw(38) << temp << pipe_v << endl;
	cout << T_r;
	printChar(38, pipe_h);
	cout << T_l << endl;
	temp = to_string(mess->dat.hour) + ":" + to_string(mess->dat.minute) + ":" + to_string(mess->dat.second);
	cout << pipe_v<<setw(38) << temp << pipe_v << endl;
	cout << T_r;
	printChar(18, pipe_h);
	cout << T_d;
	printChar(19, pipe_h);
	cout << T_l << endl;
	temp = to_string(mess->lux) + " lux";
	cout << pipe_v << setw(18) << "Lichtintensitaet" << pipe_v << setw(19) << temp << pipe_v << endl;
	printSeperator(T_u);
	cout << pipe_v << setw(38) << "DHT11_1" << pipe_v <<  endl;
	printSeperator(T_d);
	temp = to_string(mess->dht_1.temp) + " " + degree + "C";
	cout << pipe_v << setw(18) << "Temperatur" << pipe_v << setw(19) << temp << pipe_v << endl;
	printSeperator(cross);
	temp = to_string(mess->dht_1.hum) + "%";
	cout << pipe_v << setw(18) << "Luftfeuchtigkeit" << pipe_v << setw(19) << temp << pipe_v << endl;
	printSeperator(T_u);
	cout << pipe_v << setw(38) << "DHT11_2" << pipe_v << endl;
	printSeperator(T_d);
	temp = to_string(mess->dht_2.temp) + " " + degree + "C";
	cout << pipe_v << setw(18) << "Temperatur" << pipe_v << setw(19) << temp << pipe_v << endl;
	printSeperator(cross);
	temp = to_string(mess->dht_2.hum) + "%";
	cout << pipe_v << setw(18) << "Luftfeuchtigkeit" << pipe_v << setw(19) << temp << pipe_v << endl;

	cout << edge_d_l;
	printChar(18, pipe_h);
	cout << T_u;
	printChar(19, pipe_h);
	cout << edge_d_r << endl;
}
