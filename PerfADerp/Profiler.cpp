#include "Profiler.h"
#include <DbgHelp.h>
#include <sstream>
#include <iostream>

using namespace Ocuvera::CV::Profiling; 

std::unique_ptr<Profiler> Profiler::Attach(int pid)
{
	std::unique_ptr<Profiler> ret = nullptr; 
	if (::DebugActiveProcess(pid))
	{
		ret = std::unique_ptr<Profiler>(new Profiler(pid)); 
	}
	return ret; 
}

Profiler::Profiler(int pid)
{
	_pid = pid; 
}

void Profiler::Stop()
{
	_stop = true; 
	_stopped.wait(false); 

	::DebugActiveProcessStop(_pid); 
}

void Profiler::WalkStack(HANDLE hProcess, THREADENTRY32 te32)
{
	auto threadHandle = OpenThread(THREAD_GET_CONTEXT, false, te32.th32ThreadID); 
	if (threadHandle != INVALID_HANDLE_VALUE)
	{
		STACKFRAME64 stackFrame;
		::ZeroMemory(&stackFrame, sizeof(STACKFRAME64));

		std::stringstream stackTraceString;

		CONTEXT threadContext;
		::ZeroMemory(&threadContext, sizeof(threadContext)); 
		threadContext.ContextFlags = CONTEXT_FULL; 

		if (GetThreadContext(threadHandle, &threadContext))
		{
			//std::cout << "Thread " << te32.th32ThreadID << std::endl; 

			while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, // we are on a 64-bit AMD processor
				hProcess, // handle to the current process. 
				threadHandle, // handle to the current thread
				&stackFrame, // empty stack frame record, filled upon exit. 
				&threadContext, // the context record (processor-specific)
				NULL,
				SymFunctionTableAccess64, // provided by dbghelp
				SymGetModuleBase64, // provided by dbghelp
				NULL))
			{
				char szSymbolName[sizeof(IMAGEHLP_SYMBOL) + 255];
				PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)szSymbolName;
				symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
				symbol->MaxNameLength = 254;

				if (SymGetSymFromAddr64(hProcess,
					stackFrame.AddrPC.Offset,
					NULL,
					symbol))
				{
					DWORD dwOffset = 0;
					IMAGEHLP_LINE line;
					line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
					if (SymGetLineFromAddr(hProcess,
						stackFrame.AddrPC.Offset,
						&dwOffset,
						&line))
					{
						stackTraceString << line.FileName << ", " << symbol->Name << ":" << line.LineNumber << std::endl;
					}
					else
					{
						stackTraceString << "?????, " << symbol->Name << ":??????" << std::endl;
					}
				}
				else
				{
					stackTraceString << "?????, " << "??????" << ":??????" << std::endl;
				}
			}
		}

		//std::cout << stackTraceString.str() << std::endl << std::endl; 

		CloseHandle(threadHandle); 
	}
}

std::future<Result> Profiler::Start()
{
	_stop = false; 
	_stopped = false; 

	return std::async(std::launch::async, [this]()
		{
			auto processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, _pid); 

			if (processHandle != INVALID_HANDLE_VALUE)
			{
				if (SymInitialize(processHandle, NULL, true))
				{
					while(!_stop)
					{
						auto snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
						if (snapshotHandle != INVALID_HANDLE_VALUE)
						{
							THREADENTRY32 te32;
							::ZeroMemory(&te32, sizeof(te32));
							te32.dwSize = sizeof(te32);

							if (Thread32First(snapshotHandle, &te32))
							{
								do
								{
									if (te32.th32OwnerProcessID == _pid)
									{
										WalkStack(processHandle, te32);
									}

								} while (Thread32Next(snapshotHandle, &te32));
							}

							CloseHandle(snapshotHandle);
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
					}
				}

				CloseHandle(processHandle); 
			}
			
			_stopped = true; 
			_stopped.notify_one(); 

			return Result(); 
		}); 
}