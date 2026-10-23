using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using System.Windows.Input;
using ModPorter.Core;

namespace ModPorter.App;

public partial class MainWindow {
    public async Task RunLayoutTestAsync(string output) {
        var checks = new List<string>(); int scenarios = 0;
        var root = AppShell;
        string context = "startup";
        void Check(bool ok, string message) {
            if (!ok) { Capture("failure-" + context.Replace('/', '-')); throw new InvalidOperationException(context + ": " + message); }
            checks.Add(context + ": " + message);
        }
        async Task Settle() {
            await Task.Delay(25); // allow HWND resize/state messages to reach WPF
            await Dispatcher.InvokeAsync(() => { }, DispatcherPriority.ApplicationIdle);
            UpdateResponsiveLayout(); root.UpdateLayout();
            await Dispatcher.InvokeAsync(() => { }, DispatcherPriority.ApplicationIdle);
            root.UpdateLayout();
        }
        void Measure(double width, double height) {
            root.Width = width; root.Height = height;
            root.HorizontalAlignment = HorizontalAlignment.Left; root.VerticalAlignment = VerticalAlignment.Top;
            root.Measure(new Size(width, height)); root.Arrange(new Rect(0, 0, width, height)); root.UpdateLayout();
            UpdateResponsiveLayout(); root.UpdateLayout();
            Check(Math.Abs(root.ActualWidth - width) < 1 && Math.Abs(root.ActualHeight - height) < 1, "canvas has the requested effective DPI dimensions");
        }
        void Capture(string name, double scale = 1) {
            if (root.ActualWidth < 1 || root.ActualHeight < 1) return;
            var bmp = new RenderTargetBitmap((int)Math.Ceiling(root.ActualWidth * scale), (int)Math.Ceiling(root.ActualHeight * scale), 96 * scale, 96 * scale, PixelFormats.Pbgra32);
            bmp.Render(root); var png = new PngBitmapEncoder(); png.Frames.Add(BitmapFrame.Create(bmp));
            using var file = File.Create(Path.Combine(output, name + ".png")); png.Save(file);
        }
        Rect Box(FrameworkElement c) => c.TransformToAncestor(root).TransformBounds(new Rect(c.RenderSize));
        void Horizontal(FrameworkElement c) {
            var r = Box(c);
            Check(r.Width > 0 && r.Left >= 175 && r.Right <= root.ActualWidth + 1, c.Name + " stays within the page width");
            for (DependencyObject? p = VisualTreeHelper.GetParent(c); p != null && p != root; p = VisualTreeHelper.GetParent(p))
                if (p is ScrollContentPresenter viewport) {
                    var v = Box(viewport);
                    Check(r.Left >= v.Left - 1 && r.Right <= v.Right + 1, c.Name + " fits its scroll viewport horizontally");
                }
        }
        void Audit() {
            Check(Box(StatusFooter).Bottom <= root.ActualHeight + 1, "status bar remains inside window");
            Check(Box(PageHeader).Bottom <= Box(StatusFooter).Top, "header and footer do not overlap");
            var host = (FrameworkElement)Tabs.Template.FindName("PART_SelectedContentHost", Tabs);
            Check(Math.Abs(Box(host).Left - Box(PageHeader).Left) < 2, "heading and page share the same left edge");
            Check(host.ActualWidth <= 1281, "wide windows keep a readable content width");
            Check(Box(host).Bottom <= Box(StatusFooter).Top + 1, "page viewport stops above status bar");
            switch (Tabs.SelectedIndex) {
                case 0: Horizontal(InputPath); Horizontal(AnalyzeButton); break;
                case 1:
                    Horizontal(FindingsPane); Horizontal(FindingFilter); Horizontal(SeverityFilter); Horizontal(ReviewNextButton);
                    var side = SourceColumn.Visibility == Visibility.Visible ? SourceColumn : BinaryColumn;
                    Horizontal(side);
                    if (PageHeader.ActualWidth < 840) Check(Box(side).Top >= Box(FindingsPane).Bottom, "findings stack above source/details in narrow windows");
                    else Check(Box(side).Left >= Box(FindingsPane).Right, "wide findings panes do not overlap");
                    Check(FindingsList.ActualHeight >= 119, "findings list retains a usable height"); break;
                case 2:
                    Horizontal(PluginId); Horizontal(OutputPath); Horizontal(BuildButton); Horizontal(BuildNroButton); Horizontal(BuildLog);
                    Check(BuildLog.ActualHeight >= 159, "build log never collapses into a text strip");
                    Check(Box(ProjectSetup).Bottom <= Box(ProjectActions).Top + 1, "setup does not overlap build actions");
                    Check(Box(ProjectActions).Bottom <= Box(BuildOutputPanel).Top + 1, "build actions do not overlap output");
                    Check(Box(BuildOutputPanel).Bottom <= Box(ProjectFooter).Top + 1, "output does not overlap installation controls");
                    Check(Math.Max(ProjectScroll.ExtentHeight, ProjectScroll.ViewportHeight) >= ProjectLayout.ActualHeight - 1, "complete project form is scrollable"); break;
                case 3:
                    if (ToolTabs.SelectedIndex == 0) { Horizontal(SdkPath); Horizontal(PythonPath); Horizontal(DevkitPath); }
                    if (ToolTabs.SelectedIndex == 1) { Horizontal(NativeQuery); Horizontal(NativeResults); Check(NativeResults.ActualHeight >= 199, "native results retain a usable height"); }
                    if (ToolTabs.SelectedIndex == 2) { Horizontal(DiagnosticReport); Check(DiagnosticReport.ActualHeight >= 199, "diagnostic results retain a usable height"); }
                    break;
                case 4:
                    Horizontal(OpenAssetOutputButton);
                    if (AssetTabs.SelectedIndex == 0) { Horizontal(AssetInputPath); Horizontal(AssetOutputPath); Horizontal(TextureCapCombo); Horizontal(ConvertAssetsButton); if (PedAddonExpander.IsExpanded) { Horizontal(PedPackId); Horizontal(PedModelName); Horizontal(PedTemplatePath); Horizontal(PedBaseName); } }
                    if (AssetTabs.SelectedIndex == 1) { Horizontal(CurrentGamePath); Horizontal(PreparedAssetPath); if (ReplaceAssetMode.IsChecked == true) Horizontal(ArchiveEntryPath); else Horizontal(DlcId); }
                    if (AssetTabs.SelectedIndex == 2) Horizontal(AssetLog);
                    break;
            }
        }
        void Page(int page) {
            Tabs.SelectedIndex = page switch { >= 0 and <= 2 => page, >= 3 and <= 5 => 3, _ => 4 };
            if (page is >= 3 and <= 5) ToolTabs.SelectedIndex = page - 3;
            if (page >= 6) AssetTabs.SelectedIndex = page == 6 ? 0 : page == 9 ? 2 : 1;
            ReplaceAssetMode.IsChecked = page == 8; AddDlcMode.IsChecked = page != 8;
        }
        IEnumerable<T> Descendants<T>(DependencyObject p) where T : DependencyObject {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(p); i++) {
                var child = VisualTreeHelper.GetChild(p, i);
                if (child is T match) yield return match;
                foreach (var item in Descendants<T>(child)) yield return item;
            }
        }
        async Task ReachActions() {
            var host = (FrameworkElement)Tabs.Template.FindName("PART_SelectedContentHost", Tabs);
            foreach (var expander in Descendants<Expander>(host).ToArray()) expander.IsExpanded = true;
            await Settle();
            foreach (var button in Descendants<Button>(host).Where(b => b.IsVisible).ToArray()) {
                button.BringIntoView(); await Settle();
                var r = Box(button); var label = button.Content?.ToString() ?? button.Name;
                Check(r.Top >= 0 && r.Bottom <= Box(StatusFooter).Top + 1, label + " is reachable above the footer");
                for (DependencyObject? p = VisualTreeHelper.GetParent(button); p != null && p != root; p = VisualTreeHelper.GetParent(p))
                    if (p is ScrollContentPresenter v) Check(Box(v).Contains(r), label + " can scroll fully into view");
            }
            foreach (var view in Descendants<ScrollViewer>(host)) view.ScrollToTop();
            await Settle();
        }

        ShowActivated = false; ShowInTaskbar = false; Opacity = 0;
        Show(); await Settle();
        try {
            // These use a real HWND and its real client area, including the
            // non-client frame omitted by earlier screenshot-only checks.
            foreach (var size in new[] { (800d, 500d), (920d, 680d), (1200d, 850d) }) {
                WindowState = WindowState.Normal; Width = size.Item1; Height = size.Item2;
                for (int page = 0; page < 10; page++) { context = $"native-{size.Item1}x{size.Item2}-page-{page}"; Page(page); await Settle(); Check(Math.Abs(ActualWidth-size.Item1)<2 && Math.Abs(ActualHeight-size.Item2)<2,"native window has the requested size"); Audit(); scenarios++; }
            }
            Page(2); ProjectSetup.IsExpanded = true; SdInstallation.IsExpanded = true; await Settle();
            string originalInput = OutputPath.Text; int originalTab = Tabs.SelectedIndex;
            WindowState = WindowState.Maximized; await Settle(); context = "native-maximized-expanded"; Audit(); Capture(context); scenarios++;
            WindowState = WindowState.Minimized; await Settle();
            WindowState = WindowState.Normal; await Settle(); context = "native-minimize-restore"; Audit();
            Check(OutputPath.Text == originalInput && Tabs.SelectedIndex == originalTab && ProjectSetup.IsExpanded && SdInstallation.IsExpanded, "minimize/restore retains fields, navigation and expanded sections"); scenarios++;

            // Long data exercises the layout without reading game files or
            // invoking compilers, file dialogs, downloads or SD installation.
            string longPath = @"C:\Mods\" + string.Join("\\", Enumerable.Repeat("Very long source and project directory", 8)) + @"\plugin.cpp";
            foreach (TextBox box in new[] { InputPath, OutputPath, SdkPath, PythonPath, DevkitPath, AssetInputPath, AssetOutputPath, PreparedAssetPath, CurrentGamePath }) box.Text = longPath;
            PedTemplatePath.Text = longPath;
            ReviewInput.Text = ReviewTitle.Text = ProjectLabel.Text = AssetOutputLabel.Text = AssetReportSummary.Text = longPath;
            StatusText.Text = "Building: " + longPath;
            ReviewSummary.Text = "Compatibility review for a project with several source files, long filenames, and detailed next steps.";
            choices.Add(new SourceChoice { Path = longPath });
            FindingsList.ItemsSource = new[] { new Finding(Severity.Review, "LAYOUT_FIXTURE", longPath, 1234, "Review a long source path without losing the finding message or selection controls.", "Inspect the selected file, adjust platform dependencies, then check the copied source project.") };
            FindingsEmpty.Visibility = Visibility.Collapsed;
            FindingAction.Text = "Inspect the selected file, adjust platform dependencies, then check the copied source project. " + new string('w', 180);
            BuildLog.Text = NativeResults.Text = DiagnosticReport.Text = BinaryDetails.Text = AssetLog.Text = string.Join("\n", Enumerable.Repeat(longPath, 60));
            projectFile = Path.Combine(output, "fixture/project.json"); buildDirectory = Path.Combine(output, "fixture/build"); UpdateWorkspaceControls();

            // Physical pixel canvases are converted to DIPs for simulated DPI
            // layouts. Native monitor DPI changes require multiple displays.
            // Keep the visual tree loaded so TabControl content bindings and
            // item containers behave exactly as in the running application.
            // The scroll host lets a canvas exceed the test monitor's client
            // area without WPF applying the native window's layout clip to it.
            Content = null;
            Content = new ScrollViewer { Content = root, HorizontalScrollBarVisibility = ScrollBarVisibility.Auto, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };
            await Settle();
            foreach (var size in new[] { (900d, 672d, 1d), (1280d, 720d, 1d), (1920d, 1080d, 1d), (2560d, 1440d, 1d), (1920d, 1080d, 1.25d), (1920d, 1080d, 1.5d), (1920d, 1080d, 2d), (1280d, 720d, 1.5d), (1280d, 720d, 2d) }) {
                for (int page = 0; page < 10; page++) {
                    context = $"{size.Item1}x{size.Item2}-dpi{size.Item3 * 100}-page-{page}"; Page(page);
                    ProjectSetup.IsExpanded = true; SdInstallation.IsExpanded = true;
                    await Settle(); Measure(size.Item1 / size.Item3, size.Item2 / size.Item3); Audit(); scenarios++;
                    if (size is (900d, 672d, 1d) or (1920d, 1080d, 1d) or (1280d, 720d, 2d)) Capture(context, size.Item3);
                    if (size is (900d, 672d, 1d) or (1280d, 720d, 2d)) { await ReachActions(); scenarios++; }
                }
            }
            context = "nested-scroll-wheel"; Page(2); await Settle(); Measure(900, 672);
            var innerLog = Descendants<ScrollViewer>(BuildLog).Single();
            innerLog.ScrollToEnd(); ProjectScroll.ScrollToTop(); await Settle();
            innerLog.RaiseEvent(new MouseWheelEventArgs(Mouse.PrimaryDevice, Environment.TickCount, -120) { RoutedEvent = PreviewMouseWheelEvent }); await Settle();
            Check(ProjectScroll.VerticalOffset > 0, "mouse wheel leaves a log at its end and scrolls the page");
            ProjectScroll.ScrollToTop(); innerLog.ScrollToTop(); await Settle();
            innerLog.RaiseEvent(new MouseWheelEventArgs(Mouse.PrimaryDevice, Environment.TickCount, -120) { RoutedEvent = MouseWheelEvent }); await Settle();
            Check(innerLog.VerticalOffset > 0 && ProjectScroll.VerticalOffset == 0, "log scrolls normally while it has content remaining"); scenarios++;
            root.ClearValue(WidthProperty); root.ClearValue(HeightProperty);
            root.HorizontalAlignment = HorizontalAlignment.Stretch; root.VerticalAlignment = VerticalAlignment.Stretch;
            Content = null; Content = root;
            context = "busy-footer"; Page(2); Width = 920; Height = 680; WindowState = WindowState.Normal; await Settle();
            double footerHeight = StatusFooter.ActualHeight;
            BusyBar.Visibility = CancelButton.Visibility = Visibility.Visible; await Settle();
            Check(Math.Abs(StatusFooter.ActualHeight - footerHeight) < 1, "long busy status does not resize the footer"); Horizontal(CancelButton); Audit(); Capture(context); scenarios++;
            JsonFiles.Write(Path.Combine(output, "layout-test.json"), new { passed = checks.Count, failed = 0, scenarios, checks, native_window_states = new[] { "normal", "maximized", "minimized", "restored" }, simulated_dpi = new[] { 100, 125, 150, 200 }, original_input_modified = false });
        } finally { if (Content == null) Content = root; Close(); }
    }
}
