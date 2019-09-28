#include "stdafx.h"
#include "LinkLabel.h"
//#include "atlconv.h"		// for Unicode conversion

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLTIP_ID 1

// Uncomment following line to enable error message box for URL navigation
//#define XHYPERLINK_REPORT_ERROR


#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)   // From WINUSER.H
#endif

// sends message to parent when hyperlink is clicked (see SetNotifyParent())
UINT WM_XHYPERLINK_CLICKED = ::RegisterWindowMessage(_T("WM_XHYPERLINK_CLICKED"));

///////////////////////////////////////////////////////////////////////////////
// CColorLabel

BEGIN_MESSAGE_MAP(CColorLabel, CStatic)
	//{{AFX_MSG_MAP(CColorLabel)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// ctor
CColorLabel::CColorLabel()
{
	m_crTextColor	  = RGB(0,0,0);	// Purple
	m_bAdjustToFit    = TRUE;			// Resize the window to fit the text?
	m_bToolTip        = TRUE;			// TRUE = display tooltip
	m_crBackground    = (UINT) -1;		// set to default (no bg color)
}

///////////////////////////////////////////////////////////////////////////////
// dtor
CColorLabel::~CColorLabel()
{
	TRACE(_T("in CColorLabel::~CColorLabel\n"));

	if (m_Brush.GetSafeHandle())
		m_Brush.DeleteObject();
}
///////////////////////////////////////////////////////////////////////////////
// PreSubclassWindow
void CColorLabel::PreSubclassWindow() 
{
	// We want to get mouse clicks via STN_CLICKED
	DWORD dwStyle = GetStyle();
	::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);
	
	CFont* pFont = GetFont();
	if (!pFont)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT) GetStockObject(ANSI_VAR_FONT);
		if (hFont)
			pFont = CFont::FromHandle(hFont);
	}
	ASSERT(pFont->GetSafeHandle());

	// Create the underline font
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	m_StdFont.CreateFontIndirect(&lf);

	// Create the tooltip
	/*if (m_bToolTip)
	{
		CRect rect; 
		GetClientRect(rect);
		m_ToolTip.Create(this);
		m_ToolTip.AddTool(this, m_strURL, rect, TOOLTIP_ID);
	}*/

	CStatic::PreSubclassWindow();
}
///////////////////////////////////////////////////////////////////////////////
// PreTranslateMessage
BOOL CColorLabel::PreTranslateMessage(MSG* pMsg) 
{
	//m_ToolTip.RelayEvent(pMsg);
	return CStatic::PreTranslateMessage(pMsg);
}
///////////////////////////////////////////////////////////////////////////////
// SetAutoSize
void CColorLabel::SetAutoSize(BOOL bAutoSize /* = TRUE */)
{
	m_bAdjustToFit = bAutoSize;

	if (::IsWindow(GetSafeHwnd()))
		PositionWindow();
}
///////////////////////////////////////////////////////////////////////////////
// PositionWindow

// Move and resize the window so that the window is the same size
// as the hyperlink text. This stops the hyperlink cursor being active
// when it is not directly over the text. If the text is left justified
// then the window is merely shrunk, but if it is centred or right
// justified then the window will have to be moved as well.
//
// Suggested by P錶 K. T鴑der 
//
void CColorLabel::PositionWindow()
{
	if (!::IsWindow(GetSafeHwnd()) || !m_bAdjustToFit) 
		return;

	// Get the current window position
	CRect WndRect, ClientRect;
	GetWindowRect(WndRect);
	GetClientRect(ClientRect);

	ClientToScreen(ClientRect);

	CWnd* pParent = GetParent();
	if (pParent)
	{
		pParent->ScreenToClient(WndRect);
		pParent->ScreenToClient(ClientRect);
	}

	// Get the size of the window text
	CString strWndText;
	GetWindowText(strWndText);

	CDC* pDC = GetDC();
	
	CFont* pOldFont = pDC->SelectObject(GetFont());
	CSize Extent = pDC->GetTextExtent(strWndText);
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	// Adjust for window borders
	Extent.cx += WndRect.Width() - ClientRect.Width(); 
	Extent.cy += WndRect.Height() - ClientRect.Height(); 

	// Get the text justification via the window style
	DWORD dwStyle = GetStyle();

	// Recalc the window size and position based on the text justification
	if (dwStyle & SS_CENTERIMAGE)
		WndRect.DeflateRect(0, (WndRect.Height() - Extent.cy)/2);
	else
		WndRect.bottom = WndRect.top + Extent.cy;

	if (dwStyle & SS_CENTER) 
		WndRect.DeflateRect((WndRect.Width() - Extent.cx)/2, 0);
	else if (dwStyle & SS_RIGHT) 
		WndRect.left = WndRect.right - Extent.cx;
	else // SS_LEFT = 0, so we can't test for it explicitly 
		WndRect.right = WndRect.left + Extent.cx;

	// Move the window
	SetWindowPos(NULL, 
				 WndRect.left, WndRect.top, 
				 WndRect.Width(), WndRect.Height(), 
				 SWP_NOZORDER);
}
void CColorLabel::SetTextColor(COLORREF crTextColour/*= -1*/)
{
	if(crTextColour!=-1)
	{
		m_crTextColor=crTextColour;
		if (::IsWindow(GetSafeHwnd()))
			Invalidate(); 
	}
}
void CColorLabel::SetBackgroundColour(COLORREF crBackground)
{
	m_crBackground = crBackground;
	if (m_Brush.GetSafeHandle())
		m_Brush.DeleteObject();
	m_Brush.CreateSolidBrush(m_crBackground);
}
///////////////////////////////////////////////////////////////////////////////
// SetWindowText
void CColorLabel::SetWindowText(LPCTSTR lpszString)
{
	ASSERT(lpszString);
	if (!lpszString)
		return;
	CStatic::SetWindowText(_T(""));
	RedrawWindow();
	CStatic::SetWindowText(lpszString);
	//PositionWindow();自动计算Label长度
	Invalidate();
}
///////////////////////////////////////////////////////////////////////////////
// CtlColor
#ifdef _DEBUG
HBRUSH CColorLabel::CtlColor(CDC* pDC, UINT nCtlColor) 
#else
HBRUSH CColorLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/) 
#endif
{
	ASSERT(nCtlColor == CTLCOLOR_STATIC);

	pDC->SetTextColor(m_crTextColor);

	// transparent text.
	pDC->SetBkMode(TRANSPARENT);
	if (m_Brush.GetSafeHandle())
	{
		//pDC->SetBkColor(m_crBackground);
		return (HBRUSH) m_Brush;
	}
	else
	{
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
}
///////////////////////////////////////////////////////////////////////////////
// OnEraseBkgnd
BOOL CColorLabel::OnEraseBkgnd(CDC* pDC) 
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(rect, m_crBackground);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CLinkLabel

BEGIN_MESSAGE_MAP(CLinkLabel, CStatic)
	//{{AFX_MSG_MAP(CLinkLabel)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// ctor
CLinkLabel::CLinkLabel()
{
	m_hLinkCursor     = NULL;			// No cursor as yet
	m_crLinkColour    = RGB(0,0,238);	// Blue
	m_crVisitedColour = RGB(85,26,139);	// Purple
	m_crHoverColour	  = RGB(255,0,0);	// Red
	m_bOverControl    = FALSE;			// Cursor not yet over control
	m_bVisited        = FALSE;			// Hasn't been visited yet.
	m_nUnderline      = ulHover;		// Underline the link?
	m_bAdjustToFit    = TRUE;			// Resize the window to fit the text?
	m_strURL          = _T("");
	m_nTimerID        = 100;
	m_bNotifyParent   = TRUE;			// TRUE = notify parent
	m_bIsURLEnabled   = FALSE;			// TRUE = navigate to url
	m_bToolTip        = TRUE;			// TRUE = display tooltip
	m_crBackground    = (UINT) -1;		// set to default (no bg color)
	m_bAlwaysOpenNew  = FALSE;			// TRUE = always open new browser window
	m_bHoverEnabled	  = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// dtor
CLinkLabel::~CLinkLabel()
{
	TRACE(_T("in CLinkLabel::~CLinkLabel\n"));

	if (m_hLinkCursor)
		DestroyCursor(m_hLinkCursor);
	m_hLinkCursor = NULL;
	m_UnderlineFont.DeleteObject();
	if (m_Brush.GetSafeHandle())
		m_Brush.DeleteObject();
}

/////////////////////////////////////////////////////////////////////////////
// CLinkLabel overrides

///////////////////////////////////////////////////////////////////////////////
// DestroyWindow
BOOL CLinkLabel::DestroyWindow() 
{
	KillTimer(m_nTimerID);
	return CStatic::DestroyWindow();
}

///////////////////////////////////////////////////////////////////////////////
// PreTranslateMessage
BOOL CLinkLabel::PreTranslateMessage(MSG* pMsg) 
{
	m_ToolTip.RelayEvent(pMsg);
	return CStatic::PreTranslateMessage(pMsg);
}

///////////////////////////////////////////////////////////////////////////////
// PreSubclassWindow
void CLinkLabel::PreSubclassWindow() 
{
	// We want to get mouse clicks via STN_CLICKED
	DWORD dwStyle = GetStyle();
	::SetWindowLong(GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);
	
	// Set the URL as the window text
	if (m_strURL.IsEmpty())
		GetWindowText(m_strURL);

	// Check that the window text isn't empty. If it is, set it as the URL.
	CString strWndText;
	GetWindowText(strWndText);
	if (strWndText.IsEmpty()) 
	{
		ASSERT(!m_strURL.IsEmpty());	// Window and URL both NULL. DUH!
		SetWindowText(m_strURL);
	}

	CFont* pFont = GetFont();
	if (!pFont)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT) GetStockObject(ANSI_VAR_FONT);
		if (hFont)
			pFont = CFont::FromHandle(hFont);
	}
	ASSERT(pFont->GetSafeHandle());

	// Create the underline font
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	m_StdFont.CreateFontIndirect(&lf);
	lf.lfHeight=-13;
	lf.lfUnderline = (BYTE) TRUE;
	m_UnderlineFont.CreateFontIndirect(&lf);

	PositionWindow();		// Adjust size of window to fit URL if necessary
	SetDefaultCursor();		// Try and load up a "hand" cursor
	SetUnderline();

	// Create the tooltip
	if (m_bToolTip)
	{
		CRect rect; 
		GetClientRect(rect);
		m_ToolTip.Create(this);
		m_ToolTip.AddTool(this, m_strURL, rect, TOOLTIP_ID);
	}

	CStatic::PreSubclassWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CLinkLabel message handlers

///////////////////////////////////////////////////////////////////////////////
// OnClicked
void CLinkLabel::OnClicked()
{
	if (m_bIsURLEnabled)
	{
		m_bOverControl = FALSE;
		int result = HINSTANCE_ERROR + 1;
		result = (int)GotoURL(m_strURL, SW_SHOW, m_bAlwaysOpenNew);
		m_bVisited = (result > HINSTANCE_ERROR);
		if (!m_bVisited)
		{
			MessageBeep(MB_ICONEXCLAMATION);	 // Unable to follow link
			ReportError(result);
		}
		else 
			SetVisited();						// Repaint to show visited colour
	}
	//可以通过NotifyParent通知父窗口进行下一步超越操作 wjh-2016.8.17
	NotifyParent();
}

///////////////////////////////////////////////////////////////////////////////
// CtlColor
#ifdef _DEBUG
HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT nCtlColor) 
#else
HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/) 
#endif
{
	ASSERT(nCtlColor == CTLCOLOR_STATIC);

	if (m_bOverControl)
		pDC->SetTextColor(m_crHoverColour);
	else if (m_bVisited)
		pDC->SetTextColor(m_crVisitedColour);
	else
		pDC->SetTextColor(m_crLinkColour);

	// transparent text.
	pDC->SetBkMode(TRANSPARENT);
	if ((!m_bIsURLEnabled&&m_bVisited)&&m_Brush.GetSafeHandle())
	{
		//pDC->SetBkColor(m_crBackground);
		return (HBRUSH) m_Brush;
	}
	else
	{
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnMouseMove
void CLinkLabel::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bHoverEnabled&&!m_bOverControl)		// Cursor has just moved over control
	{
		m_bOverControl = TRUE;

		if (m_nUnderline == ulHover)
			SetFont(&m_UnderlineFont);
		Invalidate();

		SetTimer(m_nTimerID, 100, NULL);
	}
	CStatic::OnMouseMove(nFlags, point);
}

///////////////////////////////////////////////////////////////////////////////
// OnTimer
void CLinkLabel::OnTimer(UINT nIDEvent) 
{
	if(m_bHoverEnabled)
	{
		CPoint p(GetMessagePos());
		ScreenToClient(&p);

		CRect rect;
		GetClientRect(rect);
		if (!rect.PtInRect(p))
		{
			m_bOverControl = FALSE;
			KillTimer(m_nTimerID);

			if (m_nUnderline != ulAlways)
				SetFont(&m_StdFont);
			rect.bottom+=10;
			InvalidateRect(rect);
		}
	}
	CStatic::OnTimer(nIDEvent);
}

///////////////////////////////////////////////////////////////////////////////
// OnSetCursor
BOOL CLinkLabel::OnSetCursor(CWnd* /*pWnd*/, UINT /*nHitTest*/, UINT /*message*/) 
{
	if(!m_bHoverEnabled)
	{
		::SetCursor(LoadCursor(NULL,IDC_ARROW));//ID
		return TRUE;
	}
	else if (m_hLinkCursor)
	{
		::SetCursor(m_hLinkCursor);
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// OnEraseBkgnd
BOOL CLinkLabel::OnEraseBkgnd(CDC* pDC) 
{
	CRect rect;
	GetClientRect(rect);
	//rect.right=89;	//临时针对变电构架的，将来应统一改
	if (!m_bIsURLEnabled&&m_bVisited)//m_crBackground != (UINT)-1)
		pDC->FillSolidRect(rect, m_crBackground);
	else
		pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLinkLabel operations

///////////////////////////////////////////////////////////////////////////////
// SetURL
void CLinkLabel::SetURL(CString strURL)
{
	m_strURL = strURL;

	if (::IsWindow(GetSafeHwnd())) 
	{
		PositionWindow();
		m_ToolTip.UpdateTipText(strURL, this, TOOLTIP_ID);
	}
}

///////////////////////////////////////////////////////////////////////////////
// SetColours
void CLinkLabel::SetColours(COLORREF crLinkColour, 
							 COLORREF crVisitedColour,	
							 COLORREF crHoverColour /* = -1 */) 
{ 
	m_crLinkColour	= crLinkColour; 
	m_crVisitedColour = crVisitedColour;

	if (crHoverColour == -1)
		m_crHoverColour = ::GetSysColor(COLOR_HIGHLIGHT);
	else
		m_crHoverColour = crHoverColour;

	if (::IsWindow(m_hWnd))
		Invalidate(); 
}

///////////////////////////////////////////////////////////////////////////////
// SetBackgroundColour
void CLinkLabel::SetBackgroundColour(COLORREF crBackground)
{
	m_crBackground = crBackground;
	if (m_Brush.GetSafeHandle())
		m_Brush.DeleteObject();
	m_Brush.CreateSolidBrush(m_crBackground);
}

///////////////////////////////////////////////////////////////////////////////
// SetVisited
void CLinkLabel::SetVisited(BOOL bVisited /* = TRUE */) 
{ 
	m_bVisited = bVisited; 
	if (::IsWindow(GetSafeHwnd()))
		Invalidate(); 
}

///////////////////////////////////////////////////////////////////////////////
// SetLinkCursor
void CLinkLabel::SetLinkCursor(HCURSOR hCursor)
{ 
	m_hLinkCursor = hCursor;
	if (m_hLinkCursor == NULL)
		SetDefaultCursor();
}

///////////////////////////////////////////////////////////////////////////////
// SetUnderline
void CLinkLabel::SetUnderline(int nUnderline /*=ulHover*/)
{
	if (m_nUnderline == nUnderline)
		return;

	if (::IsWindow(GetSafeHwnd()))
	{
		if (nUnderline == ulAlways)
			SetFont(&m_UnderlineFont);
		else
			SetFont(&m_StdFont);

		Invalidate(); 
	}

	m_nUnderline = nUnderline;
}

///////////////////////////////////////////////////////////////////////////////
// SetAutoSize
void CLinkLabel::SetAutoSize(BOOL bAutoSize /* = TRUE */)
{
	m_bAdjustToFit = bAutoSize;

	if (::IsWindow(GetSafeHwnd()))
		PositionWindow();
}

///////////////////////////////////////////////////////////////////////////////
// SetWindowText
void CLinkLabel::SetWindowText(LPCTSTR lpszString)
{
	ASSERT(lpszString);
	if (!lpszString)
		return;
	CStatic::SetWindowText(_T(""));
	RedrawWindow();
	CStatic::SetWindowText(lpszString);
	PositionWindow();
	Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
// PositionWindow

// Move and resize the window so that the window is the same size
// as the hyperlink text. This stops the hyperlink cursor being active
// when it is not directly over the text. If the text is left justified
// then the window is merely shrunk, but if it is centred or right
// justified then the window will have to be moved as well.
//
// Suggested by P錶 K. T鴑der 
//
void CLinkLabel::PositionWindow()
{
	if (!::IsWindow(GetSafeHwnd()) || !m_bAdjustToFit) 
		return;

	// Get the current window position
	CRect WndRect, ClientRect;
	GetWindowRect(WndRect);
	GetClientRect(ClientRect);

	ClientToScreen(ClientRect);

	CWnd* pParent = GetParent();
	if (pParent)
	{
		pParent->ScreenToClient(WndRect);
		pParent->ScreenToClient(ClientRect);
	}

	// Get the size of the window text
	CString strWndText;
	GetWindowText(strWndText);

	CDC* pDC = GetDC();
	CFont* pOldFont = pDC->SelectObject(&m_UnderlineFont);
	CSize Extent = pDC->GetTextExtent(strWndText);
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	// Adjust for window borders
	Extent.cx += WndRect.Width() - ClientRect.Width(); 
	Extent.cy += WndRect.Height() - ClientRect.Height(); 

	// Get the text justification via the window style
	DWORD dwStyle = GetStyle();

	// Recalc the window size and position based on the text justification
	if (dwStyle & SS_CENTERIMAGE)
		WndRect.DeflateRect(0, (WndRect.Height() - Extent.cy)/2);
	else
		WndRect.bottom = WndRect.top + Extent.cy;

	if (dwStyle & SS_CENTER) 
		WndRect.DeflateRect((WndRect.Width() - Extent.cx)/2, 0);
	else if (dwStyle & SS_RIGHT) 
		WndRect.left = WndRect.right - Extent.cx;
	else // SS_LEFT = 0, so we can't test for it explicitly 
		WndRect.right = WndRect.left + Extent.cx;

	// Move the window
	SetWindowPos(NULL, 
				 WndRect.left, WndRect.top, 
				 WndRect.Width(), WndRect.Height(), 
				 SWP_NOZORDER);
}

/////////////////////////////////////////////////////////////////////////////
// CLinkLabel implementation

///////////////////////////////////////////////////////////////////////////////
// SetDefaultCursor
void CLinkLabel::SetDefaultCursor()
{
	if (m_hLinkCursor == NULL)				// No cursor handle - try to load one
	{
		// First try to load the Win98 / Windows 2000 hand cursor

		TRACE(_T("loading from IDC_HAND\n"));
		m_hLinkCursor = AfxGetApp()->LoadStandardCursor(IDC_HAND);

		if (m_hLinkCursor == NULL)			// Still no cursor handle - 
											// load the WinHelp hand cursor
		{
			// The following appeared in Paul DiLascia's Jan 1998 MSJ articles.
			// It loads a "hand" cursor from the winhlp32.exe module.

			TRACE(_T("loading from winhlp32\n"));

			// Get the windows directory
			CString strWndDir;
			GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);
			strWndDir.ReleaseBuffer();

			strWndDir += _T("\\winhlp32.exe");

			// This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
			HMODULE hModule = LoadLibrary(strWndDir);
			if (hModule) 
			{
				HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
				if (hHandCursor)
					m_hLinkCursor = CopyCursor(hHandCursor);
				FreeLibrary(hModule);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// GetRegKey
LONG CLinkLabel::GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{
	HKEY hkey;
	LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

	if (retval == ERROR_SUCCESS) 
	{
		long datasize = MAX_PATH;
		TCHAR data[MAX_PATH];
		RegQueryValue(hkey, NULL, data, &datasize);
		_tcscpy(retdata, data);
		RegCloseKey(hkey);
	}

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
// ReportError
void CLinkLabel::ReportError(int nError)
{
#ifdef XHYPERLINK_REPORT_ERROR

	CString str;
	switch (nError) 
	{
		case 0:							str = "The operating system is out\nof memory or resources."; break;
		case SE_ERR_PNF:				str = "The specified path was not found."; break;
		case SE_ERR_FNF:				str = "The specified file was not found."; break;
		case ERROR_BAD_FORMAT:			str = "The .EXE file is invalid\n(non-Win32 .EXE or error in .EXE image)."; break;
		case SE_ERR_ACCESSDENIED:		str = "The operating system denied\naccess to the specified file."; break;
		case SE_ERR_ASSOCINCOMPLETE:	str = "The filename association is\nincomplete or invalid."; break;
		case SE_ERR_DDEBUSY:			str = "The DDE transaction could not\nbe completed because other DDE transactions\nwere being processed."; break;
		case SE_ERR_DDEFAIL:			str = "The DDE transaction failed."; break;
		case SE_ERR_DDETIMEOUT:			str = "The DDE transaction could not\nbe completed because the request timed out."; break;
		case SE_ERR_DLLNOTFOUND:		str = "The specified dynamic-link library was not found."; break;
		case SE_ERR_NOASSOC:			str = "There is no application associated\nwith the given filename extension."; break;
		case SE_ERR_OOM:				str = "There was not enough memory to complete the operation."; break;
		case SE_ERR_SHARE:				str = "A sharing violation occurred. ";
		default:						str.Format(_T("Unknown Error (%d) occurred."), nError); break;
	}
	str = "Unable to open hyperlink:\n\n" + str;
	AfxMessageBox(str, MB_ICONEXCLAMATION | MB_OK);

#else

	UNUSED_ALWAYS(nError);

#endif	// XHYPERLINK_REPORT_ERROR
}

///////////////////////////////////////////////////////////////////////////////
// NotifyParent
void CLinkLabel::NotifyParent()
{
	if (m_bNotifyParent)
	{
		CWnd *pParent = GetParent();
		if (pParent && ::IsWindow(pParent->m_hWnd))
		{
			// wParam will contain control id
			pParent->SendMessage(WM_XHYPERLINK_CLICKED, GetDlgCtrlID());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// GotoURL
HINSTANCE CLinkLabel::GotoURL(LPCTSTR url, int showcmd, BOOL bAlwaysOpenNew /*= FALSE*/)
{
	// if no url then this is not an internet link
	if (!url || url[0] == _T('\0'))
		return (HINSTANCE) HINSTANCE_ERROR + 1;

	TCHAR key[MAX_PATH*2];

	// First try ShellExecute()
	TCHAR *verb = _T("open");
	if (bAlwaysOpenNew)
		verb = _T("new");
	HINSTANCE result = ShellExecute(NULL, verb, url, NULL,NULL, showcmd);

	// If it failed, get the .htm regkey and lookup the program
	if ((UINT)result <= HINSTANCE_ERROR) 
	{
		if (GetRegKey(HKEY_CLASSES_ROOT, _T(".htm"), key) == ERROR_SUCCESS) 
		{
			_tcscat(key, _T("\\shell\\open\\command"));

			if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) 
			{
				TCHAR *pos;
				pos = _tcsstr(key, _T("\"%1\""));
				if (pos == NULL) 
				{					// No quotes found
					pos = _tcsstr(key, _T("%1"));	// Check for %1, without quotes 
					if (pos == NULL)				// No parameter at all...
						pos = key + _tcslen(key)-1;
					else
						*pos = _T('\0');				// Remove the parameter
				}
				else
				{
					*pos = _T('\0');					// Remove the parameter
				}

				_tcscat(pos, _T(" "));
				_tcscat(pos, url);

				USES_CONVERSION;
				result = (HINSTANCE) WinExec(T2A(key),showcmd);
			}
		}
	}

	return result;
}

