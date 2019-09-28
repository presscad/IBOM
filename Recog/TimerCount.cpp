// TimerCount.cpp: implementation of the CTimerCount class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TimerCount.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimerCount::CTimerCount()
{
	m_dwStartTicks=m_dwEndTicks=m_dwRelayTicks=0;
	m_iStackIndex=-1;
	m_stackTicks[0]=m_stackTicks[1]=m_stackTicks[2]=m_stackTicks[3]=0;
}

CTimerCount::~CTimerCount()
{

}
bool CTimerCount::PushTicks(bool relayTicks/*=true*/)
{
	DWORD dwCurrTicks=GetTickCount();
	if(relayTicks)
		m_dwRelayTicks=dwCurrTicks;
	if(m_iStackIndex>=3)
		return false;	//固定大小的堆栈已满,不能再压栈
	m_iStackIndex++;
	m_stackTicks[m_iStackIndex]=dwCurrTicks;
	return true;
}
bool CTimerCount::PopTicks()
{
	if(m_iStackIndex<0)
		return false;
	m_dwRelayTicks=m_stackTicks[m_iStackIndex];
	m_stackTicks[m_iStackIndex]=0;
	m_iStackIndex--;
	return true;
}
DWORD CTimerCount::Start(){
	m_dwStartTicks=m_dwRelayTicks=GetTickCount();
	m_iStackIndex=-1;
	hashTicks.Empty();
	return m_dwStartTicks;
}
DWORD CTimerCount::Relay(DWORD id,DWORD dwStartTicks/*=0*/)	//临时中继性质的时间记录,id是标识
{
	DWORD tick=GetTickCount();
	if(id==0)
		m_dwRelayTicks=tick;
	else
	{
		DWORD *pTickCount=hashTicks.GetValue(id);
		DWORD relaTickCount=dwStartTicks==0?tick-m_dwRelayTicks:tick-dwStartTicks;
		m_dwRelayTicks=tick;
		if(pTickCount!=NULL)
			*pTickCount+=relaTickCount;
		else
			hashTicks.SetValue(id,relaTickCount);
	}
	return tick;
}
DWORD CTimerCount::End(){
	m_dwEndTicks=GetTickCount();
	return m_dwStartTicks;
}
