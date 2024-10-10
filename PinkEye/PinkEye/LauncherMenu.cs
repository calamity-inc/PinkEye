using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Media;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace PinkEye
{
    public partial class LauncherMenu : Form
    {
        private bool versionOutdated = false;

        private bool updateConnectionFailed = false;

        public LauncherMenu(bool local_versionOutdated, bool local_updateConnectionFailed)
        {
            versionOutdated = local_versionOutdated;
            updateConnectionFailed = local_updateConnectionFailed;

            InitializeComponent();
        }

        private void LauncherMenu_Load(object sender, EventArgs e)
        {
            if (versionOutdated == true)
            {
                comboBox1.Enabled = false;
                guna2Button1.Enabled = false;
                guna2Button1.Text = @"This version of Stand/GTAV is not currently supported.";
            }
            else if (updateConnectionFailed == true)
            {
                comboBox1.Enabled = false;
                guna2Button1.Enabled = false;
                guna2Button1.Text = @"Failed to connect to update server.";
            }
            comboBox1.DropDownStyle = ComboBoxStyle.DropDownList;
            if (Properties.Settings.Default.InjectionMode == @"Usermode")
            {
                comboBox1.SelectedItem = comboBox1.Items[0];
            }
            else if (Properties.Settings.Default.InjectionMode == @"Kernelmode")
            {
                comboBox1.SelectedItem = comboBox1.Items[1];
            }
            else
            {
                SystemSounds.Hand.Play();
                MessageBox.Show(@"There was an error while reading the ""InjectionMode"" part of the config.", Program.PinkEyeApp_Name);
                Process.GetCurrentProcess().Kill();
            }
        }

        private void guna2Button1_Click(object sender, EventArgs e)
        {
            Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
            Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");
            Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
            if (gtaBEProcessList.Length == 0 && gtaProcessList.Length == 0 && BEProcessList.Length == 0)
            {
                this.Hide();
                PinkEye pinkEye = new PinkEye();
                pinkEye.Name = Program.PinkEyeApp_Name;
                pinkEye.Text = Program.PinkEyeApp_Name;
                pinkEye.Show();
            }
            else
            {
                SystemSounds.Hand.Play();
                MessageBox.Show(@"Please close GTAV before attempting to inject.", Program.PinkEyeApp_Name);
            }
        }

        private void guna2Button2_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show(@"Are you sure that you want to uninstall?", Program.PinkEyeApp_Name, MessageBoxButtons.YesNo);

            if (result == DialogResult.Yes)
            {
                try
                {
                    //string PinkEyeDLL_DestPath = Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\" + Program.PinkEyeDLL_Name;
                    //if (File.Exists(PinkEyeDLL_DestPath) == true)
                    //{
                    //    File.Delete(PinkEyeDLL_DestPath);
                    //}

                    if (Properties.Settings.Default.Dlls.Count != 0)
                    {
                        string[] stringArray = new string[Properties.Settings.Default.Dlls.Count];
                        Properties.Settings.Default.Dlls.CopyTo(stringArray, 0);

                        foreach (string dll in stringArray)
                        {
                            if (File.Exists(dll) == true)
                            {
                                File.Delete(dll);
                            }
                        }
                    }
                    
                    if (Properties.Settings.Default.Drivers.Count != 0)
                    {
                        string[] stringArray = new string[Properties.Settings.Default.Drivers.Count];
                        Properties.Settings.Default.Drivers.CopyTo(stringArray, 0);

                        foreach (string driver in stringArray)
                        {
                            if (File.Exists(driver) == true)
                            {
                                File.Delete(driver);
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

                    Properties.Settings.Default.Reset();
                    Properties.Settings.Default.Save();

                    try
                    {
                        string PinkEye_Properties_DestPath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\PinkEye";
                        Directory.Delete(PinkEye_Properties_DestPath, true);
                    }
                    catch
                    {
                    }

                    MessageBox.Show(@"Uninstalled!");
                    Process.GetCurrentProcess().Kill();
                }
                catch
                {
                    MessageBox.Show(@"Failed to uninstall! Try closing the Rockstar Launcher and GTAV and try again.");
                }
            }
        }

        private static void ShowKernelModeInjectionWarning()
        {
            MessageBox.Show(@"**Warning: Kernel Mode Injection with PinkEye**

Before proceeding with **kernel-mode injection**, please carefully read the following:

1. **System Stability Risks**: Kernel-mode injection significantly increases the risk of system instability, crashes, and blue screen errors (BSOD). This is because kernel-level modifications interact directly with the core components of the operating system, leaving less room for error recovery.

2. **Compatibility Issues**: Certain applications and system configurations may conflict with kernel-mode injection, leading to system malfunctions or damage to your OS installation. You may also encounter hardware or driver incompatibility.

3. **Potential System Damage**: Incorrect or unauthorized kernel-level modifications may corrupt system files or cause hardware failure/issues, requiring a complete system restore or reinstallation of the operating system.

4. **Security Risks**: Running software at kernel level can expose your system to potential security vulnerabilities.

5. **Use at Your Own Risk**: By switching to kernel-mode injection, you acknowledge that you assume all risks associated with this mode of operation. **We are not responsible for any system damage, data loss, or hardware issues** that may result from the use of PinkEye in kernel-mode. 

Proceed only if you fully understand the consequences and have taken appropriate precautions such as backing up your data and system.".Replace(@"PinkEye", Program.PinkEyeApp_Name), Program.PinkEyeApp_Name);
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBox1.SelectedItem.ToString() == @"Usermode")
            {
                Properties.Settings.Default.InjectionMode = @"Usermode";
                Properties.Settings.Default.Save();
            }
            else if (comboBox1.SelectedItem.ToString() == @"Kernelmode")
            {
                Properties.Settings.Default.InjectionMode = @"Kernelmode";
                Properties.Settings.Default.Save();

                ShowKernelModeInjectionWarning();
            }
        }
    }
}
