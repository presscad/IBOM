#include "StdAfx.h"

#ifdef APP_EMBEDDED_MODULE_ENCRYPT
	#ifdef _DEBUG
	#undef THIS_FILE
	static char THIS_FILE[]=__FILE__;
	#define new DEBUG_NEW
	#endif
#endif

const int ANGLESIZE_COUNT=117;
int ANGLESIZE_NUMBERS[117] =
{
	4003,4004,4005,4503,4504,4505,4506,5003,5004,5005,
	5006,5603,5604,5605,5608,6304,6305,6306,6308,6310,
	7004,7005,7006,7007,7008,7505,7506,7507,7508,7510,
	8005,8006,8007,8008,8010,9006,9007,9008,9010,9012,
	10006,10007,10008,10010,10012,10014,10016,11007,11008,11010,
	11012,11014,12508,12510,12512,12514,14010,14012,14014,14016,
	16010,16012,16014,16016,18012,18014,18016,18018,20014,20016, 
	20018,20020,20024,22016,22018,22020,22022,22024,22026,25018,
	25020,25022,25024,25026,25028,25030,25032,25035,28020,28022,
	28024,28026,28028,28030,28032,28035,30020,30022,30024,30026,
	30028,30030,30032,30035,32022,32024,32026,32028,32030,32032,
	32035,36024,36026,36028,36030,36032,36035,
};
bool IsClassicAngleSize(int anglesize_number)
{
	int pivotindex=40,startindex=0,endindex=ANGLESIZE_COUNT;
	do{
		if(anglesize_number>ANGLESIZE_NUMBERS[pivotindex])
		{
			startindex=pivotindex;
			pivotindex=(pivotindex+endindex)/2;
			if(pivotindex==startindex)
				return false;
		}
		else if(anglesize_number==ANGLESIZE_NUMBERS[pivotindex])
			return true;
		else if(anglesize_number<ANGLESIZE_NUMBERS[pivotindex])
		{
			endindex=pivotindex;
			pivotindex=(startindex+pivotindex)/2;
		}
	}while(endindex>startindex);
	return false;
}
