using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.IO;
using Microsoft.Research.DynamicDataDisplay;
using Microsoft.Research.DynamicDataDisplay.DataSources;
using Microsoft.Research.DynamicDataDisplay.Charts.Navigation;
using Microsoft.Research.DynamicDataDisplay.ViewportConstraints;
using Microsoft.Research.DynamicDataDisplay.Charts;
using Microsoft.Research.DynamicDataDisplay.Common.Auxiliary;
using Microsoft.Research.DynamicDataDisplay.Common;
using Microsoft.Research.DynamicDataDisplay.Charts.NewLine;
using Microsoft.Research.DynamicDataDisplay.Markers2;

namespace WpfApplication1
{
    /// <summary>
    /// Interaction logic for MainWindowTest.xaml
    /// </summary>
    public partial class MainWindowTest : Window
    {
        public Random random;

        public PlotConfigWindow plotConfig;

        public List<PlotLineController> PLClist = new List<PlotLineController> { };

        public MainWindowTest()
        {
            InitializeComponent();
            random = new Random(123456);

            if(File.Exists("humanReadable.txt"))
            {
                // Load the human readable textbox
                //HumanReadableTextBox.Text = File.ReadAllText("HumanReadable.txt");
                ErrorTextBox.Text = File.ReadAllText("err.txt");

            }
            plotConfig = new PlotConfigWindow();
            plotConfig.parentWindow = this;
            FillTabs(Tabs);

            

            
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            plotConfig.Close();
            base.OnClosing(e);
        }

        

        private void removeAllChartsBtn_Click(object sender, RoutedEventArgs e)
        {
            PlotUtils.Clear_Checked_Boxes(PLClist);
            //removeAllCharts();

        }

        private void screenshotBtn_Click(object sender, RoutedEventArgs e)
        {

            //TODO
        }

        private void lockYAxisBtn_Click(object sender, RoutedEventArgs e)
        {

            //TODO
        }

        private void autoZoomBtn_Click(object sender, RoutedEventArgs e)
        {

            //TODO
        }




        public void PlotBtn_Click(object sender, RoutedEventArgs e)
        {
            PlotUtils.removeAllCharts(this.plotter);
            foreach (PlotLineController pl in PLClist)
            {
                if ((bool)pl.checkboxSelect.IsChecked)
                {
                    switch ((int)pl.Key) {

                        
                        case 0:
                            {
                                PlotUtils.plotSelectedNumericalValues(this.plotter, (string)pl.LogFilePath, (int)pl.Key, (int)pl.DataIndex, (int)pl.OffsetValue, (double)pl.ScaleValue, (string)pl.checkboxSelect.Content, (SolidColorBrush)pl.MyBrush);
                                break;
                            }
                        default:
                            {
                                PlotUtils.plotSelectedBooleanValues(this.plotter, (string)pl.LogFilePath, (int)pl.Key, (int)pl.DataIndex, (int)pl.OffsetValue, (double)pl.ScaleValue, (string)pl.checkboxSelect.Content, (SolidColorBrush)pl.MyBrush);
                                break;
                            }
                    }
                }
            }
            
        }

        public void FillTabs(TabControl tabcontrol)
        {
            // Load Logic Controller Tab
            MakeTab("Logic Controller", "lc.txt", lcDecoder.MGC_STATUS_LOOKUP, 1, 0, tabcontrol);
            MakeTab("Angle", "lc.txt", lcDecoder.MGC_BOOM_ANGLE, 2, 1, tabcontrol);
            MakeTab("Physical Inputs", "lc.txt", lcDecoder.MGC_INPUT_NAMES, 3, 0, tabcontrol);
            MakeTab("Physical Outputs", "lc.txt", lcDecoder.MGC_OUTPUT_NAMES, 4, 0, tabcontrol);
           
            // Load IO Funcs Tab
            MakeTab("Input Functions", "io.txt", lcDecoder.INPUT_FUNC_CODE_LOOKUP, 1, 0, tabcontrol);

            MakeTab("Output Functions", "io.txt", lcDecoder.OUTPUT_FUNC_CODE_LOOKUP, 2, 0, tabcontrol);

            // Load MotorGateway Tab
            MakeTab("Motor Position", "mgp.txt", mgDecoder.MGP_LOOKUP, 1, 1, tabcontrol);
            MakeTab("Motor Velocity", "mgv.txt", mgDecoder.MGV_LOOKUP, 1, 1, tabcontrol);

            // Motor Temps are from an SDO and may not be present?
            if (File.Exists("mgt.txt"))
            {
                MakeTab("Motor Temp", "mgt.txt", mgDecoder.MGT_LOOKUP, 1, 1, tabcontrol);
            }

            if (File.Exists("dm1.txt"))
            {
                MakeTab("Detector1 A", "dm1.txt", dmDecoder.LOOP_STATUS_LOOKUP, 1, 0, tabcontrol);
                MakeTab("Detector1 B", "dm1.txt", dmDecoder.LOOP_STATUS_LOOKUP, 2, 0, tabcontrol);
            }

            if (File.Exists("dm2.txt"))
            {
                MakeTab("Detector2 C", "dm2.txt", dmDecoder.LOOP_STATUS_LOOKUP, 1, 0, tabcontrol);
                MakeTab("Detector2 D", "dm2.txt", dmDecoder.LOOP_STATUS_LOOKUP, 2, 0, tabcontrol);
            }

            if (File.Exists("module.txt"))
            {
                MakeTab("Module", "module.txt", moduleDecoder.INPUT_FUNC_CODE_LOOKUP, 2, 0, tabcontrol);
            }
        }

        public void MakeTab(string name, string path, string[] STATUS_TABLE, int index, int type, TabControl tabcontrol)
        {
            TabItem Tab = new TabItem();
            Tab.Header = name;
            WrapPanel Wrap = new WrapPanel();
            ScrollViewer Scroll = new ScrollViewer();
            Scroll.Content = Wrap;

            
            for (int i = 0; i < STATUS_TABLE.Length; i++)
            {
                PlotLineController myLine;
                if (type == 0) // Boolean Values
                {
                    myLine = new PlotLineController(path, 1 << i, index, STATUS_TABLE[i], this);
                }
                else // 1 Numerical Values
                {
                    myLine = new PlotLineController(path, 0, index, STATUS_TABLE[i], this);
                }
                Wrap.Children.Add(myLine.MyPanel);
                PLClist.Add(myLine);
            }

            Tab.Content = Scroll;
            tabcontrol.Items.Add(Tab);
        }
    }
}
