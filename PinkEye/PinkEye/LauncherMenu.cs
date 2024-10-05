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
                checkBox1.Enabled = false;
                guna2Button1.Enabled = false;
                guna2Button1.Text = @"This version of Stand/GTAV is not currently supported.";
            }
            else if (updateConnectionFailed == true)
            {
                checkBox1.Enabled = false;
                guna2Button1.Enabled = false;
                guna2Button1.Text = @"Failed to connect to update server.";
            }
            checkBox1.Checked = Properties.Settings.Default.AutoInject;
        }

        private void guna2Button1_Click(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.AutoInject == true)
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
            else
            {
                Process[] gtaBEProcessList = Process.GetProcessesByName(@"GTA5_BE");
                Process[] gtaProcessList = Process.GetProcessesByName(@"GTA5");
                Process[] BEProcessList = Process.GetProcessesByName(@"BEService");
                if (gtaBEProcessList.Length == 0 && gtaProcessList.Length == 0 && BEProcessList.Length == 0)
                {
                    SystemSounds.Hand.Play();
                    MessageBox.Show(@"Please open GTAV and load into Story Mode before attempting to inject.", Program.PinkEyeApp_Name);
                }
                else
                {
                    this.Hide();
                    PinkEye pinkEye = new PinkEye();
                    pinkEye.Name = Program.PinkEyeApp_Name;
                    pinkEye.Text = Program.PinkEyeApp_Name;
                    pinkEye.Show();
                }
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

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.AutoInject = checkBox1.Checked;
            Properties.Settings.Default.Save();
        }
    }
}
