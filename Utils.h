/*
 * Utils.h
 *
 *  Created on: 08-Feb-2012
 *      Author: sandeep
 */

#ifndef UTILS_H_
#define UTILS_H_


class Utils {
public:
	Utils();
	virtual ~Utils();
	unsigned int getUnsignedIntForBytes(unsigned char bytes[4]);
	unsigned char* getBytesForUnsignedInt(unsigned int input);
	int copyBytes(char *,unsigned char *,int);
	int copyBytes(unsigned char *,unsigned char *,int);
	int copyBytes(unsigned char *, char *,int);
};

#endif /* UTILS_H_ */
