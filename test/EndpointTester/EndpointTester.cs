// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ENDPOINT ENUMERATION TESTER
// Validates that the driver endpoints are visible to the Windows Audio Engine.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using System;
using System.Linq;
using NAudio.CoreAudioApi;

namespace EndpointTester
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("=== Leyline Audio Driver Endpoint Tester (C++) ===");
            Console.WriteLine($"Starting enumeration at {DateTime.Now}...\n");

            var enumerator = new MMDeviceEnumerator();
            Console.WriteLine("--- Render Endpoints ---");
            EnumerateEndpoints(enumerator, DataFlow.Render);

            Console.WriteLine("\n--- Capture Endpoints ---");
            EnumerateEndpoints(enumerator, DataFlow.Capture);

            Console.WriteLine("\nEnumeration complete.");
        }

        static void EnumerateEndpoints(MMDeviceEnumerator enumerator, DataFlow dataFlow)
        {
            try
            {
                var endpoints = enumerator.EnumerateAudioEndPoints(dataFlow, DeviceState.All);
                int count = 0;
                foreach (var endpoint in endpoints)
                {
                    bool isLeyline = endpoint.FriendlyName.Contains("Leyline");
                    if (isLeyline || ArgsContains("-all"))
                    {
                        Console.WriteLine($"[{endpoint.State}] {endpoint.FriendlyName}");
                        Console.WriteLine($"  ID: {endpoint.ID}");
                        try {
                            Console.WriteLine($"  Format: {endpoint.AudioClient?.MixFormat}");
                        } catch (Exception ex) {
                            Console.WriteLine($"  Format Error: {ex.Message}");
                        }
                        count++;
                    }
                }
                if (count == 0)
                {
                    Console.WriteLine($"No Leyline {dataFlow} endpoints found. " +
                        "Device might be hidden, failed to bind Category, or KS topology is disjoint.");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to enumerate {dataFlow}: {ex.Message}");
            }
        }

        static bool ArgsContains(string arg) =>
            Environment.GetCommandLineArgs().Contains(arg);
    }
}
