/*
 * Utils.cpp
 *
 *  Created on: 08-Feb-2012
 *      Author: sandeep
 */

#include "Utils.h"
#include "common.h"

Utils::Utils() {
	// TODO Auto-generated constructor stub

}
unsigned char* Utils::getBytesForUnsignedInt(unsigned int input)
{
	union{
		unsigned char bytes[sizeof(input)];
		unsigned int in;
	} u;
	u.in = input;
	return u.bytes;
}

char* Utils::getBytesForInt(long long int input)
{
	union{
		char bytes[sizeof(input)];
		long long int in;
	} u;
	u.in = input;
	return u.bytes;
}

char* Utils::getBytesForInt(int input)
{
	union{
		char bytes[sizeof(input)];
		int in;
	} u;
	u.in = input;
	return u.bytes;
}




char* Utils::getBytesForKeyType(KeyType input)
{

	char * bytes;
	bytes = (char *)malloc(sizeof(KeyType));
	memcpy(bytes,&input,sizeof(input));
	return bytes;
}
KeyType  Utils::getKeyTypeForBytes(char * input)
{
	KeyType key;
	memcpy(&key,input,sizeof(key));
	return key;
}

unsigned int Utils::getUnsignedIntForBytes(unsigned char bytes[4]){
	union{
			unsigned char bytes[sizeof(unsigned int)];
			unsigned int in;
		} u;
		for ( int i = 0 ; i < sizeof(unsigned int) ; i ++)
			u.bytes[i]=bytes[i];
		return u.in;
}

int Utils::getIntForBytes(char *bytes){
	union{
			char bytes[sizeof(int)];
			int in;
		} u;

		copyBytes(u.bytes,bytes,sizeof(int));
		return u.in;
}



int Utils::copyBytes(char *destination , unsigned char * source , int number){
	for (int i = 0 ; i<number ; i++)
		destination[i] = source [i];
	return 0;
}

int Utils::copyBytes(char *destination , char * source , int number){
	for (int i = 0 ; i<number ; i++)
		destination[i] = source [i];
	return 0;
}



int Utils::copyBytes(unsigned char *destination , char * source , int number){
	for (int i = 0 ; i<number ; i++)
		destination[i] = source [i];
	return 0;
}


int Utils::copyBytes(unsigned char *destination , unsigned char * source , int number){
	for (int i = 0 ; i<number ; i++)
		destination[i] = source [i];
	return 0;
}

Utils::~Utils() {
	// TODO Auto-generated destructor stub
}

