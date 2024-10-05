using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PinkEye
{
    public partial class LauncherMenu : Form
    {
        public LauncherMenu()
        {
            InitializeComponent();
        }

        private void LauncherMenu_Load(object sender, EventArgs e)
        {
        }

        private void guna2Button1_Click(object sender, EventArgs e)
        {
            this.Hide();
            PinkEye pinkEye = new PinkEye();
            pinkEye.Show();
        }

        private void guna2Button2_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure that you want to uninstall PinkEye?", "Take antibiotics?", MessageBoxButtons.YesNo);

            if (result == DialogResult.Yes)
            {
                try
                {
                    File.Delete(Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + @"Windows\System32\PinkEye.dll");
                    Properties.Settings.Default.Reset();
                    Properties.Settings.Default.Save();
                    MessageBox.Show(@"PinkEye uninstalled!");
                    Process.GetCurrentProcess().Kill();
                }
                catch
                {
                    MessageBox.Show(@"Failed to uninstall PinkEye! Maybe try closing the Rockstar Launcher and GTAV and try again.");
                }
            }
        }
    }
}
