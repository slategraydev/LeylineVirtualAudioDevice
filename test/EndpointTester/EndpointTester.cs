// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ENDPOINT ENUMERATION TESTER
// Validates that the driver endpoints are visible to the Windows Audio Engine.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using System;
using System.Linq;
using System.Threading;
using NAudio.CoreAudioApi;
using NAudio.Wave;
using NAudio.Wave.SampleProviders;

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
                            TestFormatNegotiation(endpoint);
                            if (dataFlow == DataFlow.Render) 
                            {
                                TestPlayback(endpoint);
                                TestMultiClient(endpoint, true);
                            }
                            else 
                            {
                                TestCapture(endpoint);
                                TestMultiClient(endpoint, false);
                            }
                        } catch (Exception ex) {
                            Console.WriteLine($"  Error: {ex.Message}");
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

        static void TestFormatNegotiation(MMDevice endpoint)
        {
            Console.Write("    Testing format negotiation... ");
            try {
                using (var client = endpoint.AudioClient)
                {
                    bool supported = client.IsFormatSupported(AudioClientShareMode.Shared, new WaveFormat(48000, 32, 2));
                    Console.WriteLine($"48kHz/32-bit: {(supported ? "Supported" : "Unsupported")}");
                }
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        static void TestPlayback(MMDevice endpoint)
        {
            Console.Write("    Testing playback (200ms)... ");
            try {
                using (var waveOut = new WasapiOut(endpoint, AudioClientShareMode.Shared, true, 100))
                {
                    var provider = new SignalGenerator() { Frequency = 440, Type = SignalGeneratorType.Sin };
                    waveOut.Init(provider);
                    waveOut.Play();
                    Thread.Sleep(200);
                    waveOut.Stop();
                }
                Console.WriteLine("Passed!");
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        static void TestCapture(MMDevice endpoint)
        {
            Console.Write("    Testing capture (200ms)... ");
            try {
                using (var waveIn = new WasapiCapture(endpoint, true, 100))
                {
                    waveIn.DataAvailable += (s, e) => { };
                    waveIn.StartRecording();
                    Thread.Sleep(200);
                    waveIn.StopRecording();
                }
                Console.WriteLine("Passed!");
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        static void TestMultiClient(MMDevice endpoint, bool isRender)
        {
            Console.Write("    Testing multi-client (2 concurrent)... ");
            try {
                // Testing two concurrent streams open at once
                if (isRender)
                {
                    using (var w1 = new WasapiOut(endpoint, AudioClientShareMode.Shared, true, 100))
                    using (var w2 = new WasapiOut(endpoint, AudioClientShareMode.Shared, true, 100))
                    {
                        w1.Init(new SilenceProvider(new WaveFormat()));
                        w2.Init(new SilenceProvider(new WaveFormat()));
                        w1.Play();
                        w2.Play();
                        Thread.Sleep(50);
                    }
                }
                else
                {
                    using (var w1 = new WasapiCapture(endpoint, true, 100))
                    using (var w2 = new WasapiCapture(endpoint, true, 100))
                    {
                        w1.StartRecording();
                        w2.StartRecording();
                        Thread.Sleep(50);
                    }
                }
                Console.WriteLine("Passed!");
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        static bool ArgsContains(string arg) =>
            Environment.GetCommandLineArgs().Contains(arg);
    }
}
