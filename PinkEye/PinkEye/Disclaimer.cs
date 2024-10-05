using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PinkEye
{
    public partial class Disclaimer : Form
    {
        public Disclaimer()
        {
            InitializeComponent();
        }

        private static bool Agreed = false;

        private void Disclaimer_Load(object sender, EventArgs e)
        {

        }

        private void Disclaimer_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (Agreed == false)
            {
                Process.GetCurrentProcess().Kill();
            }
            else
            {
                Properties.Settings.Default.IsFirstLaunch = false;
                Properties.Settings.Default.Save();
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            Agreed = true;
            this.Close();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Agreed = false;
            this.Close();
        }
    }
}
