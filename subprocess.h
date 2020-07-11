#pragma once
#include <functional>

// Class to launch a sub-process and retreive output
class CSubProcess
{
public:
	using STD_OUT_CALLBACK = std::function<HRESULT(const CString&)>;

	enum RESULTS
	{
		SUCCESS							= S_OK
		, E_ABORTED						= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, 1)
		, E_PROGRAM_NOT_FOUND			= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, 2)
		, E_HR_PIPE_CREATION_ERROR		= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, 3)
		, E_HR_CREATE_PROCESS_FAILURE	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, 4)
		, S_HR_JOB_STILL_ACTIVE			= MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 5)
		, S_HR_JOB_COMPLETED			= MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 6)
	};

public:
	CSubProcess(void);
	virtual ~CSubProcess(void);

	HRESULT		Start( );
	HRESULT		LoadSettings(const CString &sProgramToExecute, const bool bHideWindow = true, const CString &programOptions = _T(""));
	HRESULT		SetStdOutCallback(STD_OUT_CALLBACK callback_function);
	HRESULT		SetStdErrCallback(STD_OUT_CALLBACK callback_function);
	HRESULT		WaitForCompletion(const DWORD maximum_wait_secs);
	HRESULT		KillSubProcess();
	CString		GetStdOut() const { return m_sStdOut; };
	CString		GetStdErr() const { return m_sErrOut; };
	DWORD		GetExitCode() const { return m_exitCode; };
	CString		GetErrorMessage() const { return m_sErrorMsg; };
	CString		GetProgramOptions() const { return m_sProgramOptions; };
	HRESULT		WriteToStdIn(const CString &sText );

protected:
	HANDLE				m_hPipeParentStdOutRd;
	HANDLE				m_hPipeChildStdOutWr;
	HANDLE				m_hPipeParentStdErrRd;
	HANDLE				m_hPipeChildStdErrWr;
	HANDLE				m_hPipeParentStdInWr;		// Std in from caller to sub-process
	HANDLE				m_hPipeChildStdInRd;
	PROCESS_INFORMATION	m_ProcessInfoChild;
	CWinThread			*m_pReadStdOutPipeThread;
	CWinThread			*m_pReadStdErrPipeThread;
	DWORD				m_milliseconds_to_timeout;

	CString				m_sProgramToExecute;
	CString				m_sProgramOptions;
	CString				m_sErrorMsg;
	bool				m_bHideWindow;

	CString				m_sStdOut;
	CString				m_sErrOut;
	DWORD				m_exitCode;						// return code from called process.
	STD_OUT_CALLBACK	m_fStdOutCallback;
	STD_OUT_CALLBACK	m_fStdErrCallback;

	static UINT _cdecl	StartStdOut_ReadPipeThread( LPVOID pParam );
	static UINT _cdecl	StartStdErr_ReadPipeThread( LPVOID pParam );
	void				ReadStdOut_PipesThread();
	void				ReadStdErr_PipesThread();
	void				CloseHandles();
	CCriticalSection	m_critSectionReadTreads;
};

