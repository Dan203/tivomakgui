#include "pch.h"
#include "subprocess.h"

/*
Copyright 2020, Daniel Rosen, DRD Systems, Inc. dba/VideoReDo

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

CSubProcess::CSubProcess(void) : 
	m_hPipeParentStdOutRd(NULL)
	, m_hPipeChildStdOutWr(NULL)
	, m_hPipeParentStdErrRd(NULL)
	, m_hPipeChildStdErrWr(NULL)
	, m_pReadStdOutPipeThread(NULL)
	, m_pReadStdErrPipeThread(NULL)
	, m_exitCode(0)
{
}

CSubProcess::~CSubProcess(void)
{
	KillSubProcess();
	CloseHandles();
}

HRESULT CSubProcess::LoadSettings(const CString &sProgramToExecute, const bool bHideWindow /* = true */, const CString &program_options /*= "" */)
{
	m_sProgramToExecute = sProgramToExecute;
	ASSERT(m_sProgramToExecute.GetLength());
	if (!m_sProgramToExecute.GetLength())
	{
		m_sErrorMsg.Format(_T("Program to execute is blank."));
		return E_PROGRAM_NOT_FOUND;
	}

	if (!::PathFileExists(m_sProgramToExecute))
	{
		m_sErrorMsg.Format(_T("Program to execute doesn't exist: %s"), m_sProgramToExecute);
		return E_PROGRAM_NOT_FOUND;
	}

	m_sProgramOptions = program_options;
	m_bHideWindow = bHideWindow;

	return S_OK;
}

HRESULT CSubProcess::Start()
{
	ASSERT(m_sProgramToExecute.GetLength());

	if (!m_sProgramToExecute.GetLength())
		return E_PROGRAM_NOT_FOUND;

	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Save the handle to the current STDOUT.  
	HANDLE hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);  

	// Create a pipe for the child process's STDOUT.  
	if(!CreatePipe(&m_hPipeParentStdOutRd, &m_hPipeChildStdOutWr, &saAttr, 0))
	{
		m_sErrorMsg.Format(_T("StdOut pipe creation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}

	// Ensure read handle to the pipe for the child's STDOUT is not inherited.
	if (!SetHandleInformation(m_hPipeParentStdOutRd, HANDLE_FLAG_INHERIT, 0))
	{
		m_sErrorMsg.Format(_T("StdOut SetHandleInformation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}

	// Create a pipe for the child process's STDERR.  
	if(!CreatePipe(&m_hPipeParentStdErrRd, &m_hPipeChildStdErrWr, &saAttr, 0))
	{
		m_sErrorMsg.Format(_T("StdOut pipe creation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}

	// Ensure read handle to the pipe for the child's STDOUT is not inherited.
	if (!SetHandleInformation(m_hPipeParentStdErrRd, HANDLE_FLAG_INHERIT, 0))
	{
		m_sErrorMsg.Format(_T("StdErr SetHandleInformation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}

	//	StdIn to child process
	if (!CreatePipe(&m_hPipeChildStdInRd, &m_hPipeParentStdInWr, &saAttr, 0))
	{
		m_sErrorMsg.Format(_T("StdIn pipe creation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}

	// Ensure read handle to the pipe for the child's STDIN is not inherited.
	if (!SetHandleInformation(m_hPipeParentStdInWr, HANDLE_FLAG_INHERIT, 0))
	{
		m_sErrorMsg.Format(_T("StdIn SetHandleInformation failed\n"));
		return E_HR_PIPE_CREATION_ERROR;
	}


	STARTUPINFO	startInfo;
	memset(&startInfo, 0, sizeof(startInfo));
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.dwFlags		|= STARTF_USESTDHANDLES;
	startInfo.hStdInput		= m_hPipeChildStdInRd;
	startInfo.hStdOutput	= m_hPipeChildStdOutWr;
	startInfo.hStdError		= m_hPipeChildStdErrWr;

	DWORD creation_flag = (m_bHideWindow) ? CREATE_NO_WINDOW : 0;

	m_ProcessInfoChild.dwProcessId = 0;
	CString sCmd;
	sCmd.Format(_T("%s %s"), m_sProgramToExecute, m_sProgramOptions);

	// Start process
	BOOL bCreateProcess = CreateProcess(NULL
		, sCmd.GetBuffer()
		, NULL
		, NULL
		, TRUE
		, creation_flag
		, NULL
		, NULL
		, &startInfo
		, &m_ProcessInfoChild
	);

	sCmd.ReleaseBuffer();

	if (!bCreateProcess)
	{
		m_sErrorMsg.Format(_T("Unable to create sub-process: %s, error: %i"), m_sProgramToExecute, GetLastError());
		return E_HR_CREATE_PROCESS_FAILURE;
	}

	/*
	* Create a job object so that if we are closed, windows will automatically clean up
	* by closing any handles we have open. When the handle to the job object
	* is closed, any processes belonging to the job will be terminated.
	* Note: Grandchild processes automatically become part of the job and
	* will also be terminated. This behaviour can be avoided by using the
	* JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK limit flag.
	*/

	HANDLE hJob = CreateJobObject(NULL, NULL);
	if (hJob)
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_extended_limit_info = {0};
		job_extended_limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &job_extended_limit_info, sizeof(job_extended_limit_info));
		AssignProcessToJobObject(hJob, m_ProcessInfoChild.hProcess);
	}

	m_pReadStdOutPipeThread = AfxBeginThread(StartStdOut_ReadPipeThread, (LPVOID) this);
	m_pReadStdErrPipeThread = AfxBeginThread(StartStdErr_ReadPipeThread, (LPVOID) this);

	//	Wait for the process to exit.  If doing a timeout, 
	return S_OK;
}

HRESULT	CSubProcess::SetStdOutCallback(STD_OUT_CALLBACK callback_function)
{
	m_fStdOutCallback = callback_function;
	return S_OK;
}

HRESULT	CSubProcess::SetStdErrCallback(STD_OUT_CALLBACK callback_function)
{
	m_fStdErrCallback = callback_function;
	return S_OK;
}

HRESULT	CSubProcess::WaitForCompletion(const DWORD maxWaitSecs)
{
	//	Waits up to xx seconds the job to complete.  
	//	Returns: S_HR_JOB_STILL_ACTIVE if job is still active
	//			 S_HR_JOB_COMPLETED if job has completed.

	DWORD rc = WaitForSingleObject(m_ProcessInfoChild.hProcess, maxWaitSecs * 1000);
	if (WAIT_TIMEOUT == rc)
		return S_HR_JOB_STILL_ACTIVE;

	GetExitCodeProcess(m_ProcessInfoChild.hProcess, &m_exitCode);
	CloseHandles();
	return S_HR_JOB_COMPLETED;
}

HRESULT	CSubProcess::KillSubProcess()
{
	//	Kill the sub process.
	//	Returns: S_HR_JOB_STILL_ACTIVE if job was previous activate and had to be terminated.
	//			 S_HR_JOB_COMPLETED if sub-process had already completed.
	if (S_HR_JOB_STILL_ACTIVE == WaitForCompletion(0))
	{
		TerminateProcess(m_ProcessInfoChild.hProcess, 32767);
		m_exitCode = 32767;
		return S_HR_JOB_STILL_ACTIVE;
	}

	return S_HR_JOB_COMPLETED;
}

HRESULT CSubProcess::WriteToStdIn(const CString &sText)
{
	DWORD bytesWritten;
	BOOL bRC = WriteFile(m_hPipeParentStdInWr, sText, sText.GetLength(), &bytesWritten, NULL);
	ASSERT(bytesWritten == sText.GetLength());

	if (!bRC)
		return E_FAIL;

	return S_OK;
}

UINT _cdecl CSubProcess::StartStdOut_ReadPipeThread(LPVOID pParam)
{
	auto pThis = (CSubProcess*) pParam;
	pThis->ReadStdOut_PipesThread();
	return 0;
}

void CSubProcess::ReadStdOut_PipesThread()
{
	//	Read std out thread, if callback defined, send the data back via the callback, otherwise append to the member variable.
	//	This assumes that the sub-process will be sending MBCS characters NOT unicode via the pipes.
	CString sReadFrame;
	while(true)
	{
		CHAR buff[1024];
		DWORD bytesRead = 0;
		BOOL bRC = ReadFile(m_hPipeParentStdOutRd, buff, (sizeof(buff)-2), &bytesRead, NULL);
		if (!bRC)
			break;

		if (bytesRead)
		{
			buff[bytesRead] = 0;		// terminate it.
			if (m_fStdOutCallback)
			{
				m_critSectionReadTreads.Lock();
				m_fStdOutCallback(CString((LPCSTR)buff));
				m_critSectionReadTreads.Unlock();
			}
			else if (m_sStdOut.GetLength() < 100000)  // Save output data up to 100K characters. Limit is there to prevent excessive slow down if the stdOut string gets really large.
				m_sStdOut.Append(CString((LPCSTR)buff));
		}
	}
}

UINT _cdecl CSubProcess::StartStdErr_ReadPipeThread(LPVOID pParam)
{
	auto	pThis = (CSubProcess *) pParam;
	pThis->ReadStdErr_PipesThread();
	return 0;
}

void CSubProcess::ReadStdErr_PipesThread()
{
	//	Read std err thread, if callback defined, send the data back via the callback, otherwise append to the member variable.
	//	This assumes that the sub-process will be sending MBCS characters NOT unicode via the pipes.
	CString sReadFrame;
	while(true)
	{
		CHAR buff[1024];
		DWORD bytesRead = 0;
		BOOL bRC = ReadFile(m_hPipeParentStdErrRd, buff, (sizeof(buff)-1), &bytesRead, NULL);
		if (!bRC)
			break;

		if (bytesRead)
		{
			buff[bytesRead] = 0;		//terminate it.
			if (m_fStdErrCallback)
			{
				m_critSectionReadTreads.Lock();
				m_fStdErrCallback(CString((LPCSTR)buff));
				m_critSectionReadTreads.Unlock();
			}
			else if (m_sErrOut.GetLength() < 100000)  //Save error data up to 100K characters. Limit is there to prevent excessive slow down if the stdErr string gets really large.
				m_sErrOut.Append(CString(buff));
		}
	}
}

void CSubProcess::CloseHandles()
{
	auto closeIt = [] (HANDLE &h) 
	{
		if (h)
		{
			CloseHandle(h);
			h = NULL;
		}
	};

	closeIt(m_hPipeChildStdOutWr);
	closeIt(m_hPipeChildStdErrWr);
	closeIt(m_hPipeParentStdOutRd);
	closeIt(m_hPipeParentStdErrRd);
}

