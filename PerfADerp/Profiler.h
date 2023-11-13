#pragma once
#include <Windows.h>
#include <memory>
#include <future>
#include <functional>
#include <tlhelp32.h>

namespace Ocuvera {
	namespace CV {
		namespace Profiling {

			struct Result
			{

			};

			class Profiler
			{
			public:
				static std::unique_ptr<Profiler> Attach(int pid);
				void Stop();
				std::future<Result> Start();
			private:
				Profiler(int pid);
				void WalkStack(HANDLE hProcess, THREADENTRY32 te32); 
				int _pid;
				std::atomic_bool _stop, _stopped; 
			};
		}
	}
}

