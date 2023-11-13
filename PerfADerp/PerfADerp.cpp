// PerfADerp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "argparse.hpp"
#include "Profiler.h"

using namespace Ocuvera::CV::Profiling; 

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("Fiber");
	program.add_argument("-i", "--processId")
		.required()
		.help("Process ID to profile.")
		.scan<'i', int>();
	program.add_argument("-d", "--duration")
		.help("How long (in ms) to sample the process.")
		.scan<'i', int>();

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program << std::endl;
		std::exit(1);
	}

	int processId = program.get<int>("-i"); 
	int duration = -1; 
	if (program.is_used("-d"))
	{
		duration = program.get<int>("-d"); 
	}

	auto prof = Profiler::Attach(processId); 
	if (prof)
	{
		auto profilerFuture = prof->Start();
		
		std::cout << "Press ENTER to stop profiling." << std::endl; 
		getchar(); 

		prof->Stop();

		profilerFuture.wait(); 
		auto result = profilerFuture.get();
	}
}
