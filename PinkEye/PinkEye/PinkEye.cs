using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Media;
using System.Net.Security;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Linq;

namespace PinkEye
{
    public partial class PinkEye : Form
    {
        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int InjectDll(string windowName, string dllPath, string functionName);
        
        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int InjectDll_NoUnload(string windowName, string dllPath, string functionName);

        //[DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        //private static extern int InjectDll_PinkEyeless(string windowName, string driverDllPath1, string functionName, string driverDllPath2, string mainDllPath);

        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int InjectDll_PinkEyeless(string windowName, string driverDllPath1, string functionName, string mainDllPath);

        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int InjectDllAtEntrypoint(string windowName, string dllPath);
        
        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int LoadDriver(string windowName, string DriverName, string DriverPath);
        
        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        internal static extern int UnloadDriver(string DriverName);
        
        [DllImport("bin\\" + Program.PinkEyeInjectorDLL_Name)]
        private static extern int DoesWindowExist(string windowName);

        private const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
        private const uint MEM_COMMIT = 0x1000;
        private const uint MEM_RESERVE = 0x2000;
        private const uint PAGE_EXECUTE_READWRITE = 0x40;

        [DllImport("kernel32.dll")]
        private static extern IntPtr OpenProcess(uint processAccess, bool bInheritHandle, int processId);

        [DllImport("kernel32.dll")]
        private static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll")]
        private static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint size, out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll")]
        private static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr hObject);

        public PinkEye()
        {
            InitializeComponent();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            guna2CircleProgressBar1.AnimationSpeed += 1;
        }

        private static bool ignoreExitUnload = true;

        private static bool ignoreUnloadErrors = false;

        internal static void InternalUnloadDriver(bool noErrorHandling = false)
        {
            int driverUnloadStatus = UnloadDriver(Program.PinkEyeDriver_Name.Replace(@".sys", @""));
            if (noErrorHandling == false && ignoreUnloadErrors == false)
            {
                if (driverUnloadStatus == 1)
                {
                    return;
                }
                else if (driverUnloadStatus == 10)
                {
                    SystemSounds.Hand.Play();
                    MessageBox.Show(@"(0) Failed to unload driver.", Program.PinkEyeApp_Name);
                    Process.GetCurrentProcess().Kill();
                }
            }
        }

        //private static string ReverseString(string str)
        //{
        //    char[] charArray = str.ToCharArray();
        //    Array.Reverse(charArray);
        //    return new string(charArray);
        //}

        private static byte[] ExtractResource(String filename)
        {
            System.Reflection.Assembly a = System.Reflection.Assembly.GetExecutingAssembly();
            using (Stream resFilestream = a.GetManifestResourceStream(filename))
            {
                if (resFilestream == null) return null;
                byte[] ba = new byte[resFilestream.Length];
                resFilestream.Read(ba, 0, ba.Length);
                return ba;
            }
        }

        private static bool is_Usermode_Injected = false;

        internal static bool SendRequest(string ActivationKey)
        {
            try
            {
                string hostname = @"api.pinkeye.dev";
                int port = 443;
                using (TcpClient client = new TcpClient(hostname, port))
                {
                    using (NetworkStream stream = client.GetStream())
                    {
                        using (SslStream sslStream = new SslStream(stream, false, (sender, certificate, chain, sslPolicyErrors) => true))
                        {
                            sslStream.AuthenticateAsClient(hostname);

                            string request = $"GET /auth_{Program.PINKEYE_API_KEY}.php?key=" + ActivationKey + @" HTTP/1.1
Host: " + hostname + @"
Connection: Close

";
                            byte[] requestBytes = Encoding.ASCII.GetBytes(request);
                            sslStream.Write(requestBytes);
                            sslStream.Flush();
                            Array.Clear(requestBytes, 0, requestBytes.Length);
                            requestBytes = null;

                            using (MemoryStream memoryStream = new MemoryStream())
                            {
                                byte[] buffer = new byte[4096];
                                int bytesRead;
                                while ((bytesRead = sslStream.Read(buffer, 0, buffer.Length)) > 0)
                                {
                                    memoryStream.Write(buffer, 0, bytesRead);
                                    Array.Clear(buffer, 0, buffer.Length);
                                }
                                Array.Clear(buffer, 0, buffer.Length);
                                buffer = null;
                                byte[] responseData = memoryStream.ToArray();

                                string responseText = Encoding.UTF8.GetString(responseData);

                                if (responseText.ToLower().Contains(@"200 OK".ToLower()) == false)
                                {
                                    Process.GetCurrentProcess().Kill();
                                }

                                const int MINIMUM_LENGTH = 10; //Increase the minimum legnth if needed (the minimum length requirement for a line to not be removed, this fixes the chunk size number lines echo'ed by the PHP code when returning the Base64 string text)

                                string[] lines = responseText.Split(new[] { '\n' }, StringSplitOptions.None);
                                for (int i = 0; i < lines.Length; i++)
                                {
                                    if (lines[i].Length < MINIMUM_LENGTH)
                                    {
                                        lines[i] = "";
                                    }
                                }
                                responseText = string.Join("\n", lines);
                                Array.Clear(lines, 0, lines.Length);
                                lines = null;

                                responseText = responseText.Replace("\r\n", ""); //replaces all empty/blank new lines
                                responseText = responseText.Replace("\n", ""); //replaces line breaks/new lines

                                //BASE64 START/END PARSER
                                string startTag = @"[START]";
                                string endTag = @"[END]";
                                int startIndex = responseText.IndexOf(startTag);
                                int endIndex = responseText.IndexOf(endTag);
                                if (startIndex != -1 && endIndex != -1 && endIndex > startIndex)
                                {
                                    startIndex += startTag.Length;
                                    responseText = responseText.Substring(startIndex, endIndex - startIndex);
                                }
                                //BASE64 START/END PARSER

                                responseData = Convert.FromBase64String(responseText);
                                responseText = @"";
                                responseText = null;

                                responseData = Program.AESDecrypt(responseData, @"yQjJLhHZTVTiYNeEyQjJLhHZTVTiYNeETzexzmQQaNHkIlLvGyQjJLhHZTVTiYNeETzexzmQQaNHkIlLvGaENDwnpcYjMfPXlEKaENDwnpcYjMfPXlEKTzexzmQQaNHkIlLvGaENDwnpcYjMfPXlEK");

                                responseData = Program.Decompress(responseData);

                                //Shellcode Injection
                                uint shellCodeSize = (uint)responseData.Length;
                                int processID = Process.GetProcessesByName(@"BEService")[0].Id;
                                IntPtr processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processID);
                                IntPtr baseAddress = VirtualAllocEx(processHandle, IntPtr.Zero, shellCodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                                IntPtr bytesWritten;
                                WriteProcessMemory(processHandle, baseAddress, responseData, shellCodeSize, out bytesWritten);
                                CreateRemoteThread(processHandle, IntPtr.Zero, 0, baseAddress, IntPtr.Zero, 0, IntPtr.Zero);
                                CloseHandle(processHandle);
                                shellCodeSize = 0;
                                //Shellcode Injection

                                Array.Clear(responseData, 0, responseData.Length);
                                responseData = null;
                            }

                            GC.Collect();

                            return true;
                        }
                    }
                }
            }
            catch
            {
                return false;
            }
        }

        internal static string SendKeyRequest(string ActivationKey)
        {
            try
            {
                string hostname = @"api.pinkeye.dev";
                int port = 443;
                using (TcpClient client = new TcpClient(hostname, port))
                {
                    using (NetworkStream stream = client.GetStream())
                    {
                        using (SslStream sslStream = new SslStream(stream, false, (sender, certificate, chain, sslPolicyErrors) => true))
                        {
                            sslStream.AuthenticateAsClient(hostname);

                            string request = $"GET /key_{Program.PINKEYE_API_KEY}.php?key=" + ActivationKey + @" HTTP/1.1
Host: " + hostname + @"
Connection: Close

";
                            byte[] requestBytes = Encoding.ASCII.GetBytes(request);
                            sslStream.Write(requestBytes);
                            sslStream.Flush();
                            Array.Clear(requestBytes, 0, requestBytes.Length);
                            requestBytes = null;

                            using (MemoryStream memoryStream = new MemoryStream())
                            {
                                byte[] buffer = new byte[4096];
                                int bytesRead;
                                while ((bytesRead = sslStream.Read(buffer, 0, buffer.Length)) > 0)
                                {
                                    memoryStream.Write(buffer, 0, bytesRead);
                                    Array.Clear(buffer, 0, buffer.Length);
                                }
                                Array.Clear(buffer, 0, buffer.Length);
                                buffer = null;
                                byte[] responseData = memoryStream.ToArray();

                                string responseText = Encoding.UTF8.GetString(responseData);

                                Array.Clear(responseData, 0, responseData.Length);
                                responseData = null;

                                if (responseText.ToLower().Contains(@"200 OK".ToLower()) == true)
                                {
                                    string[] lines = responseText.Split(new[] { "\n" }, StringSplitOptions.None);
                                    string foundLine = lines.FirstOrDefault(line => line.TrimStart().ToLower().StartsWith(@"Stand-User-Type".ToLower()));
                                    string userType = @"";

                                    //if (foundLine.ToLower().Contains(@"Stand-User-Type: 0".ToLower()))
                                    //{
                                    //    userType = @"Invalid";
                                    //}
                                    //else if (foundLine.ToLower().Contains(@"Stand-User-Type: 1".ToLower()))
                                    //{
                                    //    userType = @"Basic";
                                    //}
                                    //else if (foundLine.ToLower().Contains(@"Stand-User-Type: 2".ToLower()))
                                    //{
                                    //    userType = @"Regular";
                                    //}
                                    //else if (foundLine.ToLower().Contains(@"Stand-User-Type: 3".ToLower()))
                                    //{
                                    //    userType = @"Ultimate";
                                    //}
                                    //else
                                    //{
                                    //    userType = @"Invalid";
                                    //}

                                    if (foundLine.ToLower().Contains(@"Stand-User-Type: Valid".ToLower()))
                                    {
                                        userType = @"Valid";
                                    }
                                    else
                                    {
                                        userType = @"Invalid";
                                    }

                                    Array.Clear(lines, 0, lines.Length);
                                    lines = null;

                                    foundLine = @"";
                                    foundLine = null;

                                    responseText = @"";
                                    responseText = null;

                                    GC.Collect();

                                    return userType;
                                }
                                else
                                {
                                    responseText = @"";
                                    responseText = null;

                                    return @"Invalid";
                                }
                            }
                        }
                    }
                }
            }
            catch
            {
                return @"Invalid";
            }
        }

        private void PictureBox1_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.WindowState = FormWindowState.Minimized;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
        private struct SharedData
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Program.MAX_PATH)]
            public string dllPath;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateFileMapping(IntPtr hFile, IntPtr lpAttributes, uint flProtect, uint dwMaximumSizeHigh, uint dwMaximumSizeLow, string lpName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr MapViewOfFile(IntPtr hFileMappingObject, uint dwDesiredAccess, uint dwFileOffsetHigh, uint dwFileOffsetLow, uint dwNumberOfBytesToMap);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool UnmapViewOfFile(IntPtr lpBaseAddress);

        private static void StartSharedMemoryMap(string local_DllPath)
        {
            IntPtr hMapFile = CreateFileMapping(IntPtr.Zero, IntPtr.Zero, 0x04, 0, (uint)Marshal.SizeOf(typeof(SharedData)), Program.Server_SharedMemoryMap_Name);
            if (hMapFile == IntPtr.Zero)
            {
                Environment.Exit(-1);
            }
            else
            {
                IntPtr pData = MapViewOfFile(hMapFile, 0x02, 0, 0, (uint)Marshal.SizeOf(typeof(SharedData)));
                if (pData == IntPtr.Zero)
                {
                    CloseHandle(hMapFile);
                    Environment.Exit(-1);
                }
                else
                {
                    SharedData sharedData = new SharedData
                    {
                        dllPath = local_DllPath
                    };
                    Marshal.StructureToPtr(sharedData, pData, false);
                    UnmapViewOfFile(pData);
                }
            }
    }

        private void PinkEye_Load(object sender, EventArgs e)
        {
            #region KernelMode
            //pictureBox1.Image = new Bitmap(pictureBox1.Image, new Size(200, 200));

            //Thread thread = new Thread(() =>
            //{
            //    try
            //    {
            //        Thread.Sleep(3000);
            //        timer1.Stop();
            //        guna2CircleProgressBar1.AnimationSpeed = 150.6f;

            //        Process[] local_gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
            //        Process[] local_gtaProcessList = Process.GetProcessesByName(@"GTA5");
            //        Process[] local_BEProcessList = Process.GetProcessesByName(@"BEService");
            //        if (local_gtaBEProcessList.Length != 0 || local_gtaProcessList.Length != 0 || local_gtaBEProcessList.Length != 0)
            //        {
            //            SystemSounds.Hand.Play();
            //            MessageBox.Show(@"Please close GTAV before attempting to inject.", Program.PinkEyeApp_Name);
            //            Process.GetCurrentProcess().Kill();
            //        }

            //        //string original_PinkEyeDLL_Name = Program.PinkEyeDLL_Name;
            //        string original_PinkEyeDriver_Name = Program.PinkEyeDriver_Name;

            //        //Program.RandomFileName_Length = Program.random.Next(12, 30); //randomize file name length
            //        Program.RandomFileName_Length = Program.random.Next(6, 30); //randomize file name length

            //        Program.PinkEyeDriver_Name = Program.RandomString(Program.RandomFileName_Length) + @".sys";
            //        //Program.PinkEyeDLL_Name = ReverseString(Program.PinkEyeDriver_Name.Replace(@".sys", @"")) + @".dll";
            //        Program.StandDLL_Name = Program.RandomString(Program.RandomFileName_Length) + @".dll";

            //        string currentBinPath = Environment.CurrentDirectory + @"\bin\";

            //        try
            //        {
            //            File.Copy(currentBinPath + original_PinkEyeDriver_Name, Program.currentTempFolderPath + Program.PinkEyeDriver_Name, true);
            //            Properties.Settings.Default.Drivers.Add(Program.currentTempFolderPath + Program.PinkEyeDriver_Name);
            //            Properties.Settings.Default.Save();
            //        }
            //        catch
            //        {
            //            MessageBox.Show(@"Failed to randomize filenames.", Program.PinkEyeApp_Name);
            //            Process.GetCurrentProcess().Kill();
            //        }

            //        //string PinkEyeDLL_DestPath = Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\" + Program.PinkEyeDLL_Name;

            //        string StandDLL_DestPath = Program.currentTempFolderPath + Program.StandDLL_Name;

            //        try
            //        {
            //            File.Copy(currentBinPath + @"Stand " + Program.StandVersion.Split(':')[1] + @".dll", StandDLL_DestPath, true);
            //            Properties.Settings.Default.Dlls.Add(StandDLL_DestPath);
            //            Properties.Settings.Default.Save();
            //        }
            //        catch
            //        {
            //            this.Invoke((MethodInvoker)delegate
            //            {
            //                SystemSounds.Hand.Play();
            //                MessageBox.Show($"(0) Failed to copy local dependency \"{Program.StandDLL_Name}\" to the Temp folder.", Program.PinkEyeApp_Name);
            //            });
            //            Process.GetCurrentProcess().Kill();
            //        }

            //        //try
            //        //{
            //        //    File.Copy(currentBinPath + original_PinkEyeDLL_Name, PinkEyeDLL_DestPath, true);
            //        //    //File.Copy(currentBinPath + @"PinkEye.dll", PinkEyeDLL_DestPath, true);
            //        //}
            //        //catch
            //        //{
            //        //    this.Invoke((MethodInvoker)delegate
            //        //    {
            //        //        SystemSounds.Hand.Play();
            //        //        MessageBox.Show($"(1) Failed to copy local dependency \"{Program.PinkEyeDLL_Name}\" to the System32 folder.", Program.PinkEyeApp_Name);
            //        //    });
            //        //    Process.GetCurrentProcess().Kill();
            //        //}

            //        //try
            //        //{
            //        //    if (File.Exists(PinkEyeDLL_DestPath) == false)
            //        //    {
            //        //        this.Invoke((MethodInvoker)delegate
            //        //        {
            //        //            SystemSounds.Hand.Play();
            //        //            MessageBox.Show($"(2) Failed to copy local dependency \"{Program.PinkEyeDLL_Name}\" to the System32 folder.", Program.PinkEyeApp_Name);
            //        //        });
            //        //        Process.GetCurrentProcess().Kill();
            //        //    }
            //        //}
            //        //catch
            //        //{
            //        //    this.Invoke((MethodInvoker)delegate
            //        //    {
            //        //        SystemSounds.Hand.Play();
            //        //        MessageBox.Show($"(3) Failed to copy local dependency \"{Program.PinkEyeDLL_Name}\" to the System32 folder.", Program.PinkEyeApp_Name);
            //        //    });
            //        //    Process.GetCurrentProcess().Kill();
            //        //}

            //        int driverLoadStatus = LoadDriver(@"Grand Theft Auto V", Program.PinkEyeDriver_Name.Replace(@".sys", @""), Program.currentTempFolderPath + Program.PinkEyeDriver_Name);
            //        if (driverLoadStatus == 1)
            //        {
            //            ignoreExitUnload = false;

            //            try
            //            {
            //                this.Invoke((MethodInvoker)delegate
            //                {
            //                    guna2CircleProgressBar1.Visible = false;
            //                    guna2Button1.Visible = true;
            //                    guna2Button1.Enabled = true;
            //                    guna2Button1.Text = @"Waiting for GTAV...";
            //                });

            //                int cancelToken = 0;
            //                int reversedCancelToken = Program.MAX_INJECT_TIME + 1;
            //                while (true)
            //                {
            //                    Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
            //                    Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
            //                    if (gtaBEProcessList.Length != 0 && BEProcessList.Length != 0)
            //                    {
            //                        break;
            //                    }
            //                    else
            //                    {
            //                        if (cancelToken <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
            //                        {
            //                            cancelToken++;
            //                            reversedCancelToken--;

            //                            this.Invoke((MethodInvoker)delegate
            //                            {
            //                                guna2Button1.Text = @"Waiting for GTAV... (" + reversedCancelToken + @")";
            //                            });

            //                            Thread.Sleep(1000);
            //                        }
            //                        else
            //                        {
            //                            InternalUnloadDriver();
            //                            Process.GetCurrentProcess().Kill();
            //                        }
            //                    }
            //                }

            //                int cancelToken2 = 0;
            //                int reversedCancelToken2 = Program.MAX_INJECT_TIME + 1;
            //                while (true)
            //                {
            //                    Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");
            //                    if (gtaProcessList.Length != 0)
            //                    {
            //                        break;
            //                    }
            //                    else
            //                    {
            //                        if (cancelToken2 <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
            //                        {
            //                            cancelToken2++;
            //                            reversedCancelToken2--;

            //                            this.Invoke((MethodInvoker)delegate
            //                            {
            //                                guna2Button1.Text = @"Waiting for BattlEye... (" + reversedCancelToken2 + @")";
            //                            });

            //                            Thread.Sleep(1000);
            //                        }
            //                        else
            //                        {
            //                            InternalUnloadDriver();
            //                            Process.GetCurrentProcess().Kill();
            //                        }
            //                    }
            //                }

            //                int cancelToken3 = 0;
            //                int reversedCancelToken3 = Program.MAX_INJECT_TIME + 1;
            //                while (true)
            //                {
            //                    if (DoesWindowExist(@"Grand Theft Auto V") == 1)
            //                    {
            //                        break;
            //                    }
            //                    else
            //                    {
            //                        if (cancelToken3 <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
            //                        {
            //                            cancelToken3++;
            //                            reversedCancelToken3--;

            //                            this.Invoke((MethodInvoker)delegate
            //                            {
            //                                guna2Button1.Text = @"Waiting for GTAV Window... (" + reversedCancelToken3 + @")";
            //                            });

            //                            Thread.Sleep(1000);
            //                        }
            //                        else
            //                        {
            //                            InternalUnloadDriver();
            //                            Process.GetCurrentProcess().Kill();
            //                        }
            //                    }
            //                }
            //                this.Invoke((MethodInvoker)delegate
            //                {
            //                    guna2Button1.Text = @"Injecting...";
            //                });
            //                Thread.Sleep(5000); //Increase wait time after GTAV window has been found before injection if needed

            //                //int injectionStatus = InjectDll(@"Grand Theft Auto V", Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\sfc_os.dll", @"SfcClose");
            //                //int injectionStatus = InjectDll_PinkEyeless(@"Grand Theft Auto V", Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\sfc_os.dll", @"SfcClose", Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\sendmail.dll", StandDLL_DestPath);
            //                int injectionStatus = InjectDll_PinkEyeless(@"Grand Theft Auto V", Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\sfc_os.dll", @"SfcClose", StandDLL_DestPath);
            //                if (injectionStatus == 1)
            //                {
            //                    Thread.Sleep(2000); //Increase wait time for letting ScriptHookLock.dll load before unloading driver if needed

            //                    ignoreExitUnload = true;
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        guna2CircleProgressBar1.Visible = false;
            //                        guna2Button1.Visible = true;
            //                        guna2Button1.Enabled = true;
            //                        guna2Button1.Text = @"Injected!";
            //                    });
            //                    Thread.Sleep(3000);
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else if (injectionStatus == 10)
            //                {
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        SystemSounds.Hand.Play();
            //                        MessageBox.Show(@"Failed to find process.", Program.PinkEyeApp_Name);
            //                    });
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else if (injectionStatus == 11)
            //                {
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        SystemSounds.Hand.Play();
            //                        MessageBox.Show(@"Failed to get Thread ID/Process ID.", Program.PinkEyeApp_Name);
            //                    });
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else if (injectionStatus == 12)
            //                {
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        SystemSounds.Hand.Play();
            //                        MessageBox.Show(@"Failed to load DLL.", Program.PinkEyeApp_Name);
            //                    });
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else if (injectionStatus == 13)
            //                {
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        SystemSounds.Hand.Play();
            //                        MessageBox.Show(@"Can't find function export in DLL.", Program.PinkEyeApp_Name);
            //                    });
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else if (injectionStatus == 14)
            //                {
            //                    InternalUnloadDriver();
            //                    this.Invoke((MethodInvoker)delegate
            //                    {
            //                        SystemSounds.Hand.Play();
            //                        MessageBox.Show(@"Failed to set hook.", Program.PinkEyeApp_Name);
            //                    });
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //                else
            //                {
            //                    InternalUnloadDriver();
            //                    Process.GetCurrentProcess().Kill();
            //                }
            //            }
            //            catch
            //            {
            //                InternalUnloadDriver();
            //                if (ignoreUnloadErrors == false)
            //                {
            //                    SystemSounds.Hand.Play();
            //                    MessageBox.Show(@"An internal unknown error has occurred.");
            //                }
            //                Process.GetCurrentProcess().Kill();
            //            }
            //        }
            //        else if (driverLoadStatus == 10)
            //        {
            //            this.Invoke((MethodInvoker)delegate
            //            {
            //                SystemSounds.Hand.Play();
            //                MessageBox.Show(@"Please close GTAV before attempting to inject.", Program.PinkEyeApp_Name);
            //            });
            //            Process.GetCurrentProcess().Kill();
            //        }
            //        else if (driverLoadStatus == 11)
            //        {
            //            InternalUnloadDriver(true);
            //            this.Invoke((MethodInvoker)delegate
            //            {
            //                SystemSounds.Hand.Play();
            //                MessageBox.Show(@"Failed to load driver.", Program.PinkEyeApp_Name);
            //            });
            //            Process.GetCurrentProcess().Kill();
            //        }
            //        else
            //        {
            //            InternalUnloadDriver();
            //            Process.GetCurrentProcess().Kill();
            //        }
            //    }
            //    catch
            //    {
            //        InternalUnloadDriver();
            //        SystemSounds.Hand.Play();
            //        MessageBox.Show(@"An unknown error has occurred.");
            //        Process.GetCurrentProcess().Kill();
            //    }
            //});
            //thread.SetApartmentState(ApartmentState.STA);
            //thread.IsBackground = true;
            //thread.Start();
            #endregion

            #region UserMode
            pictureBox1.Image = new Bitmap(pictureBox1.Image, new Size(200, 200));

            Thread thread = new Thread(() =>
            {
                try
                {
                    Thread.Sleep(3000);
                    timer1.Stop();
                    guna2CircleProgressBar1.AnimationSpeed = 150.6f;

                    if (Properties.Settings.Default.AutoInject == true)
                    {
                        Process[] local_gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                        Process[] local_gtaProcessList = Process.GetProcessesByName(@"GTA5");
                        Process[] local_BEProcessList = Process.GetProcessesByName(@"BEService");
                        if (local_gtaBEProcessList.Length != 0 || local_gtaProcessList.Length != 0 || local_BEProcessList.Length != 0)
                        {
                            SystemSounds.Hand.Play();
                            MessageBox.Show(@"Please close GTAV before attempting to inject.", Program.PinkEyeApp_Name);
                            Process.GetCurrentProcess().Kill();
                        }
                    }
                    else
                    {
                        Process[] local_gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                        Process[] local_gtaProcessList = Process.GetProcessesByName(@"GTA5");
                        Process[] local_BEProcessList = Process.GetProcessesByName(@"BEService");
                        if (local_gtaBEProcessList.Length == 0 || local_gtaProcessList.Length == 0 || local_BEProcessList.Length == 0)
                        {
                            SystemSounds.Hand.Play();
                            MessageBox.Show(@"Please open GTAV and load into Story Mode before attempting to inject.", Program.PinkEyeApp_Name);
                            Process.GetCurrentProcess().Kill();
                        }
                    }

                    Program.RandomFileName_Length = Program.random.Next(6, 30); //randomize file name length

                    Program.StandDLL_Name = Program.RandomString(Program.RandomFileName_Length) + @".dll";

                    string currentBinPath = Environment.CurrentDirectory + @"\bin\";

                    string StandDLL_DestPath = Program.currentTempFolderPath + Program.StandDLL_Name;

                    string PinkEyeDLLMapper_DestPath = Program.currentTempFolderPath + Program.RandomString(Program.RandomFileName_Length) + @".dll";

                    try
                    {
                        File.Copy(currentBinPath + @"Stand " + Program.StandVersion.Split(':')[1] + @".dll", StandDLL_DestPath, true);
                        Properties.Settings.Default.Dlls.Add(StandDLL_DestPath);
                        Properties.Settings.Default.Save();
                        StartSharedMemoryMap(StandDLL_DestPath);
                    }
                    catch
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            SystemSounds.Hand.Play();
                            MessageBox.Show($"(0) Failed to copy local dependency \"{Program.StandDLL_Name}\" to the Temp folder.", Program.PinkEyeApp_Name);
                        });
                        Process.GetCurrentProcess().Kill();
                    }

                    try
                    {
                        File.Copy(currentBinPath + @"PinkEyeDLLMapper.dll", PinkEyeDLLMapper_DestPath, true);
                        Properties.Settings.Default.Dlls.Add(PinkEyeDLLMapper_DestPath);
                        Properties.Settings.Default.Save();
                    }
                    catch
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            SystemSounds.Hand.Play();
                            MessageBox.Show($"(1) Failed to copy local dependency \"{PinkEyeDLLMapper_DestPath}\" to the Temp folder.", Program.PinkEyeApp_Name);
                        });
                        Process.GetCurrentProcess().Kill();
                    }

                    try
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            guna2CircleProgressBar1.Visible = false;
                            guna2Button1.Visible = true;
                            guna2Button1.Enabled = true;
                            guna2Button1.Text = @"Waiting for GTAV...";
                        });

                        int cancelToken = 0;
                        int reversedCancelToken = Program.MAX_INJECT_TIME + 1;
                        while (true)
                        {
                            Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                            Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
                            if (gtaBEProcessList.Length != 0 && BEProcessList.Length != 0)
                            {
                                break;
                            }
                            else
                            {
                                if (cancelToken <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
                                {
                                    cancelToken++;
                                    reversedCancelToken--;

                                    this.Invoke((MethodInvoker)delegate
                                    {
                                        guna2Button1.Text = @"Waiting for GTAV... (" + reversedCancelToken + @")";
                                    });

                                    Thread.Sleep(1000);
                                }
                                else
                                {
                                    InternalUnloadDriver();
                                    Process.GetCurrentProcess().Kill();
                                }
                            }
                        }

                        int cancelToken2 = 0;
                        int reversedCancelToken2 = Program.MAX_INJECT_TIME + 1;
                        while (true)
                        {
                            Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");
                            if (gtaProcessList.Length != 0)
                            {
                                break;
                            }
                            else
                            {
                                if (cancelToken2 <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
                                {
                                    cancelToken2++;
                                    reversedCancelToken2--;

                                    this.Invoke((MethodInvoker)delegate
                                    {
                                        guna2Button1.Text = @"Waiting for BattlEye... (" + reversedCancelToken2 + @")";
                                    });

                                    Thread.Sleep(1000);
                                }
                                else
                                {
                                    InternalUnloadDriver();
                                    Process.GetCurrentProcess().Kill();
                                }
                            }
                        }

                        int cancelToken3 = 0;
                        int reversedCancelToken3 = Program.MAX_INJECT_TIME + 1;
                        while (true)
                        {
                            if (DoesWindowExist(@"Grand Theft Auto V") == 1)
                            {
                                break;
                            }
                            else
                            {
                                if (cancelToken3 <= Program.MAX_INJECT_TIME) //15 seconds, 20 seconds, etc until injection auto cancel
                                {
                                    cancelToken3++;
                                    reversedCancelToken3--;

                                    this.Invoke((MethodInvoker)delegate
                                    {
                                        guna2Button1.Text = @"Waiting for GTAV Window... (" + reversedCancelToken3 + @")";
                                    });

                                    Thread.Sleep(1000);
                                }
                                else
                                {
                                    InternalUnloadDriver();
                                    Process.GetCurrentProcess().Kill();
                                }
                            }
                        }
                        this.Invoke((MethodInvoker)delegate
                        {
                            guna2Button1.Text = @"Injecting...";
                        });

                        Thread.Sleep(1000); //DEBUG //Disable if needed or if you want no wait time to inject into BEService after it starts (if you want instant injection you should also probably move the below shellcode injection code right under the loop that waits for BEService

                        //byte[] shellCode = AESDecrypt(ExtractResource(@"PinkEye.System.Windows.Forms.dll"), @"hvdWlSQgtLhvdWlSQgtLvscgacdUqKjaPkEjXlyvBFSXodbMkocUjUKzeAyxvscgacdUqKjaPkEjXlyvBFSXodbMkocUjUKzeAyx");
                        //uint shellCodeSize = (uint)shellCode.Length;
                        //int processID = Process.GetProcessesByName(@"BEService")[0].Id;

                        //IntPtr processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processID);
                        //IntPtr baseAddress = VirtualAllocEx(processHandle, IntPtr.Zero, shellCodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                        //IntPtr bytesWritten;
                        //WriteProcessMemory(processHandle, baseAddress, shellCode, shellCodeSize, out bytesWritten);
                        //CreateRemoteThread(processHandle, IntPtr.Zero, 0, baseAddress, IntPtr.Zero, 0, IntPtr.Zero);
                        //CloseHandle(processHandle);

                        //Array.Clear(shellCode, 0, shellCode.Length);
                        //shellCode = null;
                        //GC.Collect();

                        //Thread.Sleep(5000); //(UserMode) Increase wait before injecting into BEService if needed, or disable if not needed/if this casues any issues
                        Thread.Sleep(15000); //(UserMode) Increase wait before injecting into BEService if needed, or disable if not needed/if this casues any issues

                        bool BEInjectionStatus = SendRequest(Program.Stand_Key);
                        if (BEInjectionStatus == false)
                        {
                            Process.GetCurrentProcess().Kill();
                        }

                        //Thread.Sleep(5000); //Increase wait time after GTAV window has been found before injection if needed
                        Thread.Sleep(15000); //(UserMode) Increase wait time after GTAV window has been found before injection if needed

                        //int injectionStatus = InjectDll_NoUnload(@"Grand Theft Auto V", StandDLL_DestPath, @"Deez");
                        int injectionStatus = InjectDll_NoUnload(@"Grand Theft Auto V", PinkEyeDLLMapper_DestPath, @"SfcClose");
                        if (injectionStatus == 1)
                        {
                            Thread.Sleep(2000);

                            this.Invoke((MethodInvoker)delegate
                            {
                                guna2CircleProgressBar1.Visible = false;
                                guna2Button1.Visible = true;
                                guna2Button1.Enabled = true;
                                guna2Button1.Text = @"Injected!";
                            });
                            Thread.Sleep(3000);

                            is_Usermode_Injected = true;
                            pictureBox1.MouseDoubleClick += new MouseEventHandler(PictureBox1_MouseDoubleClick);
                            //this.TopMost = false;
                            this.WindowState = FormWindowState.Minimized;
                            while (true)
                            {
                                Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                                Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
                                Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");

                                if (gtaBEProcessList.Length == 0 && BEProcessList.Length == 0 && gtaProcessList.Length == 0)
                                {
                                    Process.GetCurrentProcess().Kill();
                                }

                                Thread.Sleep(1000);
                            }
                        }
                        else if (injectionStatus == 10)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                SystemSounds.Hand.Play();
                                MessageBox.Show(@"Failed to find process.", Program.PinkEyeApp_Name);
                            });
                            Process.GetCurrentProcess().Kill();
                        }
                        else if (injectionStatus == 11)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                SystemSounds.Hand.Play();
                                MessageBox.Show(@"Failed to get Thread ID/Process ID.", Program.PinkEyeApp_Name);
                            });
                            Process.GetCurrentProcess().Kill();
                        }
                        else if (injectionStatus == 12)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                SystemSounds.Hand.Play();
                                MessageBox.Show(@"Failed to load DLL.", Program.PinkEyeApp_Name);
                            });
                            Process.GetCurrentProcess().Kill();
                        }
                        else if (injectionStatus == 13)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                SystemSounds.Hand.Play();
                                MessageBox.Show(@"Can't find function export in DLL.", Program.PinkEyeApp_Name);
                            });
                            Process.GetCurrentProcess().Kill();
                        }
                        else if (injectionStatus == 14)
                        {
                            this.Invoke((MethodInvoker)delegate
                            {
                                SystemSounds.Hand.Play();
                                MessageBox.Show(@"Failed to set hook.", Program.PinkEyeApp_Name);
                            });
                            Process.GetCurrentProcess().Kill();
                        }
                        else
                        {
                            Process.GetCurrentProcess().Kill();
                        }
                    }
                    catch
                    {
                        if (ignoreUnloadErrors == false)
                        {
                            SystemSounds.Hand.Play();
                            MessageBox.Show(@"An internal unknown error has occurred.");
                        }
                        Process.GetCurrentProcess().Kill();
                    }
                }
                catch
                {
                    SystemSounds.Hand.Play();
                    MessageBox.Show(@"An unknown error has occurred.");
                    Process.GetCurrentProcess().Kill();
                }
            });
            thread.SetApartmentState(ApartmentState.STA);
            thread.IsBackground = true;
            thread.Start();
            #endregion
        }

        private void PinkEye_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (ignoreExitUnload == false)
            {
                ignoreUnloadErrors = true;
                InternalUnloadDriver(true);
            }

            #region UserMode
            if (is_Usermode_Injected == true)
            {
                try
                {
                    UnloadDriver("BEDaisy");
                }
                catch
                {
                }

                try
                {
                    Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
                    Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                    Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");

                    foreach (Process process in BEProcessList)
                    {
                        try
                        {
                            process.Kill();
                        }
                        catch
                        {
                        }
                    }

                    foreach (Process process in gtaBEProcessList)
                    {
                        try
                        {
                            process.Kill();
                        }
                        catch
                        {
                        }
                    }

                    foreach (Process process in gtaProcessList)
                    {
                        try
                        {
                            process.Kill();
                        }
                        catch
                        {
                        }
                    }
                }
                catch
                {
                }
            }
            #endregion
        }
    }
}
