/*****************************************************************************
*	Filename: pcadpcm.c                                                      *
******************************************************************************
*	Author:  Rodger Richey                                                   *
*	Title:  Senior Applications Engineer                                     *
*	Company: Microchip Technology Incorporated                               *
*	Revision: 0                                                              *
*	Date: 1-11-96                                                            *
*	Compiled using Borland C+ Version 3.1                                    *
******************************************************************************
*	Include files:                                                           *
*		 stdio.h - Standard input/output header file                         *
*		 pcadpcm.h - ADPCM related information header file (Rev0)            *
******************************************************************************
*	This file contains the ADPCM encode and decode routines. These           *
*	routines were obtained from the Interactive Multimedia Association's     *
*   Reference ADPCM algorithm. This algorithm was first implemented by       *
*	Intel/DVI.                                                               *
*****************************************************************************/

#include <stdio.h>
#include "pcadpcm.h"

/* Table of index changes */
signed char IndexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

signed int IndexTable3bit[8] = {
	-1, -1, 1, 2,
	-1, -1, 1, 2
};

signed int IndexTable2Bit[4] = { -1, 2, -1, 2};

signed int IndexTable5Bit[32] = {
	-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16,
	-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16
};

/* Quantizer step size lookup table */
int StepSizeTable[89] = {
	 7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

/*****************************************************************************
 *	ADPCMEncoder - ADPCM encoder routine                                     *
 *****************************************************************************
 *	Input variables:                                                         *
 *		short sample - 16-bit signed speech sample                           *
 *		struct ADPCMstate *state - ADPCM structure                           *
 *	Return variables:	                                                     *
 *		char - 8-bit number containing the 4-bit ADPCM code	                 *
 *****************************************************************************/
char ADPCMEncoder( short sample, int bits, struct ADPCMstate *state )
{
	int code;			/* ADPCM output value */
	int diff;			/* Difference between sample and the predicted sample */
	int step;			/* Quantizer step size */
	int predsample;		/* Output of ADPCM predictor */
	//int diffq;			/* Dequantized predicted difference */
	int index;			/* Index into step size table */

	/* Restore previous values of predicted sample and quantizer step size index*/

	predsample = (int)(state->prevsample);
	index = state->previndex;
	step = StepSizeTable[index];

	/* Compute the difference between the acutal sample (sample) and the he predicted sample (predsample)*/

	diff = sample - predsample;

		if(diff < 0) {
			switch(bits) {
			case 5:
				code = 16;
				break;
			case 4:
				code = 8;
				break;
			case 3:
				code = 4;
				break;
			case 2:
				code = 2;
				break;
			}

			diff = -diff;
		}
		else
			code = 0;

		/* Quantize the difference into the 4-bit ADPCM code using the the quantizer step size */

		if( diff >= step)
		{
			switch(bits) {
			case 5:
				code |= 8;
				break;
			case 4:
				code |= 4;
				break;
			case 3:
				code |= 2;
				break;
			case 2:
				code |= 1;
				break;
			}

			diff -= step;
		}
		if(diff >= (step >> 1))
		{
			switch(bits) {
			case 5:
				code |= 4;
				diff -= (step >> 1);
				break;
			case 4:
				code |= 2;
				diff -= (step >> 1);
				break;
			case 3:
				code |= 1;
				break;
			}
		}

		if(diff >= (step >> 2)) {
			switch(bits) {
			case 5:
				code |= 2;
				diff -= (step >> 2);
				break;
			case 4:
				code |= 1;
			}
		}

		if(bits == 5 && diff >= (step >> 3))
			code |= 1;


		/* Inverse quantize the ADPCM code into a predicted difference using the quantizer step size */
		diff = 0;

		if(((bits == 4) && (code & 4)) || ((bits == 3) && (code & 2)) || ((bits == 5) && (code & 8)) || ((bits == 2) && (code & 1)))
			diff += step;


		if(((bits == 4) && (code & 2)) || ((bits == 3) && (code & 1)) || ((bits == 5) && (code & 4)))
			diff += (step >> 1);


		if(((bits == 4) && (code & 1)) || ((bits == 5) && (code & 2)))
			diff += (step >> 2);

		if((bits == 5) && (code & 1))
			diff += (step >> 3);

		if(bits ==4)
			diff += (step >> 3);
		else if(bits == 2)
			diff += (step >> 1);
		else if(bits == 3)
			diff += (step >> 2);
		else
			diff += (step >> 4);

		if((bits == 4 && (code & 8)) || (bits == 3 && (code & 4)) || ((bits == 5) && (code & 16)) || (bits == 2 && (code & 2)))
			predsample -= diff;
		else
			predsample += diff;

	/* Fixed predictor computes new predicted sample by adding the old predicted sample to predicted difference */

	/* Check for overflow of the new predicted sample */
	if( predsample > 32767 )
		predsample = 32767;
	else if( predsample < -32767 )
		predsample = -32767;

	/* Find new quantizer stepsize index by adding the old index to a table lookup using the ADPCM code */
	if(bits == 4)
		index += IndexTable[code];
	else if(bits == 3)
		index += IndexTable3bit[code];
	else if(bits == 2)
		index += IndexTable2Bit[code];
	else
		index += IndexTable5Bit[code];

	/* Check for overflow of the new quantizer step size index */
	if( index < 0 )
		index = 0;
	if( index > 88 )
		index = 88;

	/* Save the predicted sample and quantizer step size index for next iteration */
	state->prevsample = (short)predsample;
	state->previndex = index;

	if(bits == 4)
		code &= 0x0f;
	else if (bits == 3)
		code &= 0x07;
	else if (bits == 2)
		code &= 0x03;
	else
		code &= 0x1f;

	/* Return the new ADPCM code */
	return code ;
}

/*****************************************************************************
 *	ADPCMDecoder - ADPCM decoder routine                                     *
 *****************************************************************************
 *	Input variables:                                                         *
 *		char code - 8-bit number containing the 4-bit ADPCM code             *
 *		struct ADPCMstate *state - ADPCM structure                           *
 *	Return variables:	                                                     *
 *		int - 16-bit signed speech sample	                                 *
 *****************************************************************************/
int ADPCMDecoder(char code, int bits, struct ADPCMstate *state)
{
	int step;	/* Quantizer step size */
	int predsample;	/* Output of ADPCM predictor */
	int diff;	/* Dequantized predicted difference */
	int index;/* Index into step size table */

	/* Restore previous values of predicted sample and quantizer step size index */
	predsample = (int)(state->prevsample);
	index = state->previndex;

	/* Find quantizer step size from lookup table using index */
	step = StepSizeTable[index];

	/* Inverse quantize the ADPCM code into a difference using the quantizer step size */

		diff = 0;
		if(((bits == 4) && (code & 4)) || ((bits == 3) && (code & 2)) || ((bits == 5) && (code & 8)) || ((bits == 2) && (code & 1)))
			diff += step;

		if(((bits == 4) && (code & 2)) || ((bits == 3) && (code & 1)) || ((bits == 5) && (code & 4)))
			diff += (step >> 1);

		if(((bits == 4) && (code & 1)) || ((bits == 5) && (code & 2)))
			diff += (step >> 2);

		if((bits == 5) && (code & 1))
			diff += (step >> 3);


		if(bits ==4)
			diff += (step >> 3);
		else if(bits == 2)
			diff += (step >> 1);
		else if(bits == 3)
			diff += (step >> 2);
		else
			diff += (step >> 4);


		if((bits == 4 && (code & 8)) || ((bits == 3) && (code & 4)) ||  ((bits == 5) && (code & 16)) ||  ((bits == 2) && (code & 2)))
			predsample -= diff;
		else
			predsample += diff;

	/* Check for overflow of the new predicted sample */
	if( predsample > 32767)
		predsample = 32767;
	else if( predsample < -32768)
		predsample = -32768;

	/* Find new quantizer step size by adding the old index and a table lookup using the ADPCM code */
	if(bits == 4)
		index += IndexTable[code];
	else if(bits == 3)
		index += IndexTable3bit[code];
	else if(bits == 2)
		index += IndexTable2Bit[code];
	else
		index += IndexTable5Bit[code];

	/* Check for overflow of the new quantizer step size index */
	if( index < 0)
		index = 0;
	if( index > 88)
		index = 88;

	/* Save predicted sample and quantizer step size index for next iteration*/
	state->prevsample = (short)predsample;
	state->previndex = index;

	/* Return the new speech sample */
	return( predsample );
}

