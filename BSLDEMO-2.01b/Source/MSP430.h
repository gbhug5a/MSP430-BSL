/**
 * @mainpage MSP430 BSL
 *
 * @section intro Introduction
 *
*/



/**            
 * \file MSP430.h
 * 
 *
 */
#ifndef _MSP430_H_
#define _MSP430_H_

// #includes. -----------------------------------------------------------------
#include <windows.h>

// #defines. ------------------------------------------------------------------

typedef unsigned char BYTE;
typedef unsigned short WORD;


#define RET_ERR(x)   return(x)
#define RET_OK       return(NO_ERR)



/// this is the definition for the DLL functions return value
typedef LONG STATUS_T;

/**
 \brief  Status codes of the DLL functions.
*/
enum STATUS_CODE 
{
   /// DLL functions return this value on failure
    STATUS_ERROR = -1,
   /// this value is returned on success
    STATUS_OK,
};


/// File types.
enum FILE_TYPE
{
   /// Auto detect.
    FILETYPE_AUTO,      
   /// TI text.
    FILETYPE_TI_TXT,    
   /// Intel hex.
    FILETYPE_INTEL_HEX, 
};




#endif // _MSP430_H_
