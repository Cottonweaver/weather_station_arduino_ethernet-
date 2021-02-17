#pragma once
struct datum
{
	int second;
	int minute;
	int hour;
	int day;
	int month;
	int year;
};

struct DHT11
{
	float temp;
	float hum;
};

struct messung
{
	datum dat;
	DHT11 dht_1;
	DHT11 dht_2;
	float lux;
};
