// TimerCount.h: interface for the CTimerCount class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMERCOUNT_H__76CA18AB_703B_4964_9D14_EBB9D3849C45__INCLUDED_)
#define AFX_TIMERCOUNT_H__76CA18AB_703B_4964_9D14_EBB9D3849C45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HashTable.h"
class CTimerCount  
{
	int   m_iStackIndex;
	DWORD m_stackTicks[4];
	DWORD m_dwStartTicks,m_dwEndTicks,m_dwRelayTicks;
public:
	CHashList<DWORD>hashTicks;
	CTimerCount();
	virtual ~CTimerCount();
	DWORD Start();
	bool PushTicks(bool relayTicks=true);
	bool PopTicks();
	DWORD GetStartTicks(){return m_dwStartTicks;}
	DWORD GetEndTicks(){return m_dwEndTicks;}
	//临时中继性质的时间记录,id是分类统计时间标识,dwStartTicks>0时表示自dwStartTicks到当前的时间计入到id分类中
	DWORD Relay(DWORD id=0,DWORD dwStartTicks=0);
	DWORD End();
};

#endif // !defined(AFX_TIMERCOUNT_H__76CA18AB_703B_4964_9D14_EBB9D3849C45__INCLUDED_)
