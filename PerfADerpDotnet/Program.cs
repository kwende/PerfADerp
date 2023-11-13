using Microsoft.Diagnostics.Runtime;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace PerfADerpDotnet
{
    internal class Program
    {
        class TraceInfo
        {
            public ClrStackFrame[] StackTrace;
            public int Count;
        }

        static void Main(string[] args)
        {
            Dictionary<string, TraceInfo> pairs = new Dictionary<string, TraceInfo>();

            Console.Write("Number of times to scan: ");
            int scanCount = int.Parse(Console.ReadLine());
            Console.WriteLine("Scanning...");

            for (int i = 0; i < scanCount; i++)
            {
                using (DataTarget dt = DataTarget.AttachToProcess(37136, true))
                {
                    var hostRuntime = dt.ClrVersions[0].CreateRuntime();

                    foreach (ClrThread thread in hostRuntime.Threads)
                    {
                        StringBuilder searchableString = new StringBuilder();
                        // Enumerate stack frames
                        IEnumerable<ClrStackFrame> frames = thread.EnumerateStackTrace();
                        foreach (ClrStackFrame frame in frames)
                        {
                            if (frame.Method != null)
                            {
                                searchableString.AppendLine(frame.Method.Name);
                            }
                            else
                            {
                                searchableString.AppendLine("???");
                            }
                        }

                        string searchableStringString = searchableString.ToString();
                        if (pairs.ContainsKey(searchableStringString))
                        {
                            pairs[searchableStringString].Count++;
                        }
                        else
                        {
                            pairs.Add(searchableStringString, new TraceInfo
                            {
                                Count = 1,
                                StackTrace = frames.ToArray<ClrStackFrame>(),
                            });
                        }
                    }
                }

                Thread.Sleep(TimeSpan.FromSeconds(1));
            }

            foreach (var pair in pairs)
            {
                Console.WriteLine($"Happened {pair.Value.Count} times");

                foreach (var stack in pair.Value.StackTrace)
                {
                    Console.WriteLine(stack.ToString());
                }

                Console.WriteLine("======");
            }
        }
    }
}
