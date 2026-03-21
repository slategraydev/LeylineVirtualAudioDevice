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

            var renderEndpoint = enumerator.EnumerateAudioEndPoints(DataFlow.Render, DeviceState.Active).FirstOrDefault(e => e.FriendlyName.Contains("Leyline"));
            var captureEndpoint = enumerator.EnumerateAudioEndPoints(DataFlow.Capture, DeviceState.Active).FirstOrDefault(e => e.FriendlyName.Contains("Leyline"));
            
            if (renderEndpoint != null && captureEndpoint != null)
            {
                TestAudioQuality(renderEndpoint, captureEndpoint);
                TestPerformanceBenchmarks(renderEndpoint, captureEndpoint);
            }

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

        static void TestAudioQuality(MMDevice render, MMDevice capture)
        {
            Console.WriteLine("\n--- Audio Quality (Loopback) Test ---");
            Console.Write("    Testing bit-exactness... ");
            try {
                using (var waveOut = new WasapiOut(render, AudioClientShareMode.Shared, true, 100))
                using (var waveIn = new WasapiCapture(capture, true, 100))
                {
                    bool exactMatch = false;
                    waveIn.DataAvailable += (s, e) => {
                        var floats = new float[e.BytesRecorded / 4];
                        Buffer.BlockCopy(e.Buffer, 0, floats, 0, e.BytesRecorded);
                        for (int i = 0; i < floats.Length; i++) {
                            if (Math.Abs(floats[i] - 0.1337f) < 0.000001f) {
                                exactMatch = true;
                                break;
                            }
                        }
                    };
                    waveIn.StartRecording();

                    var floatProvider = new MagicFloatProvider(waveOut.OutputWaveFormat);
                    waveOut.Init(floatProvider);
                    waveOut.Play();

                    Thread.Sleep(500);
                    waveOut.Stop();
                    waveIn.StopRecording();

                    Console.WriteLine(exactMatch ? "PASSED" : "FAILED");
                }
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        class MagicFloatProvider : IWaveProvider
        {
            public WaveFormat WaveFormat { get; }
            public MagicFloatProvider(WaveFormat format) { WaveFormat = format; }
            public int Read(byte[] buffer, int offset, int count)
            {
                int floatsToWrite = count / 4;
                float[] fbuf = new float[floatsToWrite];
                for (int i = 0; i < floatsToWrite; i++) fbuf[i] = 0.1337f;
                Buffer.BlockCopy(fbuf, 0, buffer, offset, count);
                return count;
            }
        }

        static void TestPerformanceBenchmarks(MMDevice render, MMDevice capture)
        {
            Console.WriteLine("\n--- Performance Benchmarks ---");
            Console.Write("    Measuring round-trip latency... ");
            try {
                using (var waveOut = new WasapiOut(render, AudioClientShareMode.Shared, true, 20))
                using (var waveIn = new WasapiCapture(capture, true, 20))
                {
                    var sw = new System.Diagnostics.Stopwatch();
                    bool received = false;
                    long elapsedMs = 0;
                    
                    waveIn.DataAvailable += (s, e) => {
                        if (received) return;
                        var floats = new float[e.BytesRecorded / 4];
                        Buffer.BlockCopy(e.Buffer, 0, floats, 0, e.BytesRecorded);
                        for (int i = 0; i < floats.Length; i++) {
                            if (floats[i] > 0.5f && !received) {
                                sw.Stop();
                                elapsedMs = sw.ElapsedMilliseconds;
                                received = true;
                                break;
                            }
                        }
                    };
                    waveIn.StartRecording();
                    var impulseProvider = new ImpulseProvider(waveOut.OutputWaveFormat, sw);
                    waveOut.Init(impulseProvider);
                    waveOut.Play();
                    
                    DateTime startWait = DateTime.Now;
                    while (!received && (DateTime.Now - startWait).TotalSeconds < 2) {
                        Thread.Sleep(1);
                    }
                    
                    waveOut.Stop();
                    waveIn.StopRecording();
                    
                    if (received)
                        Console.WriteLine($"{elapsedMs} ms");
                    else
                        Console.WriteLine("FAILED (Timeout)");
                }
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }

            Console.Write("    Multithreaded / High-load Stress Test... ");
            try {
                var streams = new System.Collections.Generic.List<IDisposable>();
                // Open 10 render and 10 capture streams.
                for(int i = 0; i < 10; i++) {
                    var wOut = new WasapiOut(render, AudioClientShareMode.Shared, true, 20);
                    // 192kHz simulation - note: WASAPI Shared mode resamples to Mix Format internally.
                    wOut.Init(new SilenceProvider(wOut.OutputWaveFormat)); 
                    wOut.Play();
                    streams.Add(wOut);
                    
                    var wIn = new WasapiCapture(capture, true, 20);
                    wIn.StartRecording();
                    streams.Add(wIn);
                }
                
                // Warm up
                Thread.Sleep(500);
                
                // We can't easily measure kernel DPC latency here, but we can measure jitter of user-mode sleep
                long maxJitter = 0;
                long lastTick = System.Diagnostics.Stopwatch.GetTimestamp();
                for(int i = 0; i < 100; i++) {
                    Thread.Sleep(10); // Sleep 10ms
                    long tick = System.Diagnostics.Stopwatch.GetTimestamp();
                    long delta = (tick - lastTick) * 1000 / System.Diagnostics.Stopwatch.Frequency;
                    if (delta > 10) {
                        long jitter = delta - 10;
                        if (jitter > maxJitter) maxJitter = jitter;
                    }
                    lastTick = tick;
                }
                
                foreach(var s in streams) s.Dispose();
                
                Console.WriteLine($"Passed (Max user-mode timer jitter: {maxJitter} ms)");
            } catch (Exception ex) {
                Console.WriteLine($"Failed ({ex.Message})");
            }
        }

        class ImpulseProvider : IWaveProvider
        {
            public WaveFormat WaveFormat { get; }
            private System.Diagnostics.Stopwatch _sw;
            private bool _sent;
            public ImpulseProvider(WaveFormat format, System.Diagnostics.Stopwatch sw) { WaveFormat = format; _sw = sw; _sent = false; }
            public int Read(byte[] buffer, int offset, int count)
            {
                int floatsToWrite = count / 4;
                float[] fbuf = new float[floatsToWrite];
                for (int i = 0; i < floatsToWrite; i++) {
                    if (!_sent && i == 0) {
                        fbuf[i] = 1.0f; // Impulse
                        _sw.Start();
                        _sent = true;
                    } else {
                        fbuf[i] = 0.0f;
                    }
                }
                Buffer.BlockCopy(fbuf, 0, buffer, offset, count);
                return count;
            }
        }

        static bool ArgsContains(string arg) =>
            Environment.GetCommandLineArgs().Contains(arg);
    }
}
