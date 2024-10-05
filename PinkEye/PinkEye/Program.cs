using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PinkEye
{
    internal static class Program
    {
        internal static string PinkEyeDLL_Name = @"PinkEye.dll";
        internal static string PinkEyeDriver_Name = @"PinkEye.sys";
        internal const string PinkEyeInjectorDLL_Name = @"PinkEyeInjector.dll";
        internal const string PinkEyeApp_Name = @"PinkEye";
        internal static string StandDLL_Name = @"";

        internal static string currentTempFolderPath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Temp\";
        
        //internal static string StandVersion = @"1.9.3:24.9.9";
        internal static string StandVersion = @"";

        internal const string DeadManSwitch = @"1.4.4.5";

        internal const string PINKEYE_API_KEY = @"wRqzrDtSnKOuCqmEEEuyuEWLrePuzYZqxaqvbJGyBskUeWEpoA";

        internal const string Mutex_Name = @"Global\vHabKURbBmfYgGZxBPlepkVxQFNvbBvHabKURbBmfYgGZxBPlepkVxQFNvbBaZSjLAbexZxICxraaMcAaZSjLAbexZxICxraaMcA";

        internal const int MAX_INJECT_TIME = 100;

        internal static int RandomFileName_Length = 12;

        internal static string Server_SharedMemoryMap_Name = @"vJenGTEfTdKtUvJenGTEfTdKtUzOVIcBFAFrDTHHxYfmAEhBjmRagSMtwupfmpmzOVIcBFAFrDTHHxYfmAEhBjmvJenGTEfTdKtUvJenGTEfTdKtUzOVIcBFAFrDTHHxYfmAEhBjmRagSMtwupfmpmzOVIcBFAFrDTHHxYfmAEhBjmRagSMtwupfmpmRagSMtwupfmpm";

        internal const int MAX_PATH = 260;

        internal static Random random = new Random();

        internal static string userType = @"";
        internal static string Stand_Key = @"";

        internal static string RandomString(int length)
        {
            const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
            return new string(Enumerable.Repeat(chars, length).Select(s => s[random.Next(s.Length)]).ToArray());
        }
        
        internal static string RandomNumber(int length)
        {
            const string chars = "1234567890";
            return new string(Enumerable.Repeat(chars, length).Select(s => s[random.Next(s.Length)]).ToArray());
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool SetProcessDPIAware();

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            SetProcessDPIAware();

            bool createdNew;
            Mutex mutex = new Mutex(false, Mutex_Name, out createdNew);
            if (!createdNew)
            {
                MessageBox.Show(@"PinkEye is either currently open already, or currently injected into a GTA5 instance.", @"PinkEye");
                Process.GetCurrentProcess().Kill();
            }

            try
            {
                string ServerIPAddress = Dns.GetHostAddresses(@"dms.pinkeye.dev")[0].ToString();
                if (ServerIPAddress != DeadManSwitch)
                {
                    MessageBox.Show(@"This version of BattlEye/GTAV is not currently supported.", PinkEyeApp_Name);
                    Process.GetCurrentProcess().Kill();
                }
            }
            catch
            {
                MessageBox.Show(@"This version of BattlEye/GTAV is not currently supported.", PinkEyeApp_Name);
                Process.GetCurrentProcess().Kill();
            }

            try
            {
                string StandAppData_Path = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\Stand\";
                if (Directory.Exists(StandAppData_Path) == true)
                {
                    string StandKeyFile_Path = StandAppData_Path + @"Activation Key.txt";
                    if (File.Exists(StandKeyFile_Path) == true)
                    {
                        string standKey = File.ReadAllText(StandKeyFile_Path);
                        standKey = standKey.Replace(@"Stand-Activate-", @"");

                        userType = PinkEye.SendKeyRequest(standKey);
                        if (userType == @"Invalid")
                        {
                            MessageBox.Show(@"Failed to verify the Stand Activation Key.", PinkEyeApp_Name);
                            Process.GetCurrentProcess().Kill();
                        }
                        else
                        {
                            Stand_Key = standKey;
                        }
                    }
                    else
                    {
                        MessageBox.Show(@"Failed to find the Stand Activation Key.", PinkEyeApp_Name);
                        Process.GetCurrentProcess().Kill();
                    }
                }
                else
                {
                    MessageBox.Show(@"Failed to find the Stand installation.", PinkEyeApp_Name);
                    Process.GetCurrentProcess().Kill();
                }
            }
            catch
            {
                MessageBox.Show(@"Failed to verify with Stand Auth servers.", PinkEyeApp_Name);
                Process.GetCurrentProcess().Kill();
            }

            bool versionOutdated = false;
            bool updateConnectionFailed = false;

            try
            {
                WebClient webClient = new WebClient();
                string currentVersion = webClient.DownloadString(@"https://stand.sh/versions.txt");
                //if (currentVersion.Split(':')[1] != StandVersion.Split(':')[1])
                //{
                //    versionOutdated = true;
                //}

                StandVersion = currentVersion;
                string currentBinPath = Environment.CurrentDirectory + @"\bin\";
                string[] standDlls = Directory.GetFiles(currentBinPath, @"Stand *.dll".ToLower(), SearchOption.TopDirectoryOnly);
                if (standDlls.Length > 1)
                {
                    foreach (string standDllFile in standDlls)
                    {
                        try
                        {
                            File.Delete(standDllFile);
                        }
                        catch
                        {
                        }
                    }
                    standDlls = Directory.GetFiles(currentBinPath, @"Stand *.dll".ToLower(), SearchOption.TopDirectoryOnly);
                }
                string firstDll = standDlls.FirstOrDefault();
                if (standDlls.Length == 0 || Path.GetFileNameWithoutExtension(firstDll).Split(' ')[1] != currentVersion.Split(':')[1])
                {
                    try
                    {
                        File.Delete(firstDll);
                    }
                    catch
                    {
                    }
                    string newDllName = @"Stand " + currentVersion.Split(':')[1] + @".dll";
                    byte[] standDllBytes = Decompress(AESDecrypt(webClient.DownloadData(@"https://api.pinkeye.dev/downloads/" + Uri.EscapeUriString(newDllName)), @"xZdVINjjxNwFmDxQmYCObUKhisXsarINJKRaNmLCFrZZGGvNUB"));
                    File.WriteAllBytes(currentBinPath + newDllName, standDllBytes);
                    Array.Clear(standDllBytes, 0, standDllBytes.Length);
                    standDllBytes = null;
                    GC.Collect();
                }
            }
            catch (Exception ex)
            {
                if (ex.ToString().ToLower().Contains(@"(404) Not Found".ToLower()) == true)
                {
                    versionOutdated = true;
                }
                else
                {
                    updateConnectionFailed = true;
                }
            }

            try
            {
                PinkEye.InternalUnloadDriver(true);
            }
            catch
            {
            }

            try
            {
                if (Properties.Settings.Default.Drivers == null)
                {
                    Properties.Settings.Default.Drivers = new System.Collections.Specialized.StringCollection();
                    Properties.Settings.Default.Save();
                }
                if (Properties.Settings.Default.Dlls == null)
                {
                    Properties.Settings.Default.Dlls = new System.Collections.Specialized.StringCollection();
                    Properties.Settings.Default.Save();
                }
            }
            catch
            {
                MessageBox.Show(@"Failed to initialize settings.", PinkEyeApp_Name);
                Process.GetCurrentProcess().Kill();
            }

            try
            {
                if (Properties.Settings.Default.Dlls.Count != 0)
                {
                    string[] stringArray = new string[Properties.Settings.Default.Dlls.Count];
                    Properties.Settings.Default.Dlls.CopyTo(stringArray, 0);

                    foreach (string dll in stringArray)
                    {
                        try
                        {
                            if (File.Exists(dll) == true)
                            {
                                File.Delete(dll);
                            }
                        }
                        catch
                        {
                        }
                    }
                }

                if (Properties.Settings.Default.Drivers.Count != 0)
                {
                    string[] stringArray = new string[Properties.Settings.Default.Drivers.Count];
                    Properties.Settings.Default.Drivers.CopyTo(stringArray, 0);

                    foreach (string driver in stringArray)
                    {
                        try
                        {
                            if (File.Exists(driver) == true)
                            {
                                File.Delete(driver);
                            }
                        }
                        catch
                        {
                        }
                        try
                        {
                            PinkEye.UnloadDriver(Path.GetFileNameWithoutExtension(driver));
                        }
                        catch
                        {
                        }
                    }
                }
            }
            catch
            {
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            if (Properties.Settings.Default.IsFirstLaunch == true)
            {
                Form disclaimerForm = new Disclaimer();
                disclaimerForm.Name = PinkEyeApp_Name;
                disclaimerForm.Text = PinkEyeApp_Name;
                Application.Run(disclaimerForm);
            }
            Form launchermenuForm = new LauncherMenu(versionOutdated, updateConnectionFailed);
            launchermenuForm.Name = PinkEyeApp_Name;
            launchermenuForm.Text = PinkEyeApp_Name;
            Application.Run(launchermenuForm);
        }

        internal static byte[] AESDecrypt(byte[] input, string Pass)
        {
            System.Security.Cryptography.RijndaelManaged AES = new System.Security.Cryptography.RijndaelManaged();
            byte[] hash = new byte[32];
            byte[] temp = new MD5CryptoServiceProvider().ComputeHash(System.Text.Encoding.ASCII.GetBytes(Pass));
            Array.Copy(temp, 0, hash, 0, 16);
            Array.Copy(temp, 0, hash, 15, 16);
            AES.Key = hash;
            AES.Mode = System.Security.Cryptography.CipherMode.ECB;
            System.Security.Cryptography.ICryptoTransform DESDecrypter = AES.CreateDecryptor();
            return DESDecrypter.TransformFinalBlock(input, 0, input.Length);
        }

        internal static byte[] Decompress(byte[] data)
        {
            using (MemoryStream input = new MemoryStream(data))
            {
                using (MemoryStream output = new MemoryStream())
                {
                    using (DeflateStream dstream = new DeflateStream(input, CompressionMode.Decompress))
                    {
                        dstream.CopyTo(output);
                    }
                    return output.ToArray();
                }
            }
        }
    }
}
