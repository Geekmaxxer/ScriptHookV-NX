using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using System.Windows.Input;
using System.Windows.Media;

namespace ModPorter.App;

public partial class MainWindow {
    bool layoutRefreshQueued;

    void InitializeResponsiveLayout() {
        PreviewMouseWheel += ForwardBoundaryWheel;
        AppShell.SizeChanged += (_, _) => QueueResponsiveLayout();
        foreach (var view in new[] { ProjectScroll, ReviewScroll, NativeScroll, DiagnosticsScroll, AssetPageScroll })
            view.SizeChanged += (_, _) => QueueResponsiveLayout();
        foreach (FrameworkElement element in new FrameworkElement[] { ProjectSetup, ProjectActions, ProjectFooter, ReviewCard, AssetOutputFooter })
            element.SizeChanged += (_, _) => QueueResponsiveLayout();
        ToolTabs.SelectionChanged += (_, _) => QueueResponsiveLayout();
        AssetTabs.SelectionChanged += (_, _) => QueueResponsiveLayout();
        Loaded += (_, _) => {
            // Keep the first launch inside a laptop's work area. Subsequent
            // resizing and monitor changes remain under normal window control.
            if (!TestMode) {
                var area = SystemParameters.WorkArea;
                double maxWidth = Math.Max(1, area.Width - 24), maxHeight = Math.Max(1, area.Height - 24);
                MinWidth = Math.Min(MinWidth, maxWidth); MinHeight = Math.Min(MinHeight, maxHeight);
                if (Width > maxWidth || Height > maxHeight) {
                    Width = Math.Min(Width, maxWidth); Height = Math.Min(Height, maxHeight);
                    Left = area.Left + (area.Width - Width) / 2; Top = area.Top + (area.Height - Height) / 2;
                }
            }
            QueueResponsiveLayout();
        };
    }

    static DependencyObject? LayoutParent(DependencyObject element) => element is Visual
        ? VisualTreeHelper.GetParent(element) : (element as FrameworkContentElement)?.Parent;

    void ForwardBoundaryWheel(object sender, MouseWheelEventArgs e) {
        if (e.Handled || e.Delta == 0 || Keyboard.Modifiers != ModifierKeys.None) return;
        bool foundInner = false;
        for (DependencyObject? p = e.OriginalSource as DependencyObject; p != null; p = LayoutParent(p)) {
            if (p is not ScrollViewer view) continue;
            bool canMove = view.ScrollableHeight > 0 && (e.Delta > 0 ? view.VerticalOffset > 0 : view.VerticalOffset < view.ScrollableHeight);
            if (!foundInner) { if (canMove) return; foundInner = true; continue; }
            if (!canMove) continue;
            // Let a parent page consume the wheel only when the inner list or
            // output is at its limit. Normal list/log scrolling stays native.
            e.Handled = true;
            view.RaiseEvent(new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta) { RoutedEvent = MouseWheelEvent, Source = view });
            return;
        }
    }

    void QueueResponsiveLayout() {
        if (layoutRefreshQueued) return;
        layoutRefreshQueued = true;
        Dispatcher.BeginInvoke(DispatcherPriority.Loaded, new Action(() => {
            layoutRefreshQueued = false;
            UpdateResponsiveLayout();
        }));
    }

    void UpdateResponsiveLayout() {
        if (AppShell.ActualWidth <= 0 || AppShell.ActualHeight <= 0) return;
        double frameHeight = Math.Max(1, AppShell.ActualHeight - PageHeader.ActualHeight - PageHeader.Margin.Top - PageHeader.Margin.Bottom - StatusFooter.ActualHeight - 24);
        double ViewHeight(ScrollViewer view, double tabs = 0) => view.ViewportHeight > 0 && double.IsFinite(view.ViewportHeight) ? view.ViewportHeight : Math.Max(1, frameHeight - tabs);
        void Height(FrameworkElement control, double value) { if (Math.Abs(control.Height - value) > 0.5 || double.IsNaN(control.Height)) control.Height = value; }

        bool stackedFields = PageHeader.ActualWidth < 640;
        ProjectFields.ColumnDefinitions[0].Width = stackedFields ? new GridLength(1, GridUnitType.Star) : new GridLength(150);
        ProjectFields.ColumnDefinitions[1].Width = new GridLength(stackedFields ? 0 : 14);
        ProjectFields.ColumnDefinitions[2].Width = stackedFields ? new GridLength(0) : new GridLength(1, GridUnitType.Star);
        PluginIdField.Width = stackedFields ? 220 : double.NaN;
        PluginIdField.HorizontalAlignment = stackedFields ? HorizontalAlignment.Left : HorizontalAlignment.Stretch;
        Grid.SetColumn(ProjectFolderField, stackedFields ? 0 : 2); Grid.SetRow(ProjectFolderField, stackedFields ? 1 : 0);
        Grid.SetColumnSpan(ProjectFolderField, stackedFields ? 3 : 1);
        ProjectFolderField.Margin = new Thickness(0, stackedFields ? 14 : 0, 0, 0);

        // A log must stay readable even when the form needs to scroll. Expanded
        // sections consume page extent rather than shrinking the log to zero.
        double reserved = ProjectSetup.ActualHeight + ProjectSetup.Margin.Top + ProjectSetup.Margin.Bottom
            + ProjectActions.ActualHeight + ProjectFooter.ActualHeight + BuildOutputPanel.Margin.Top;
        Height(BuildOutputPanel, Math.Max(220, ViewHeight(ProjectScroll) - reserved));

        bool narrow = PageHeader.ActualWidth < 840;
        var columns = ReviewPanels.ColumnDefinitions;
        columns[0].Width = new GridLength(narrow ? 1 : 3, GridUnitType.Star);
        columns[1].Width = new GridLength(narrow ? 0 : 16);
        columns[2].Width = narrow ? new GridLength(0) : new GridLength(2, GridUnitType.Star);
        double bodyHeight = Math.Max(380, ViewHeight(ReviewScroll) - ReviewCard.ActualHeight - ReviewCard.Margin.Bottom);
        Height(FindingsPane, bodyHeight);
        foreach (var pane in new FrameworkElement[] { SourceColumn, BinaryColumn }) {
            Grid.SetRow(pane, narrow ? 1 : 0); Grid.SetColumn(pane, narrow ? 0 : 2);
            pane.Margin = new Thickness(0, narrow ? 18 : 0, 0, 0);
            Height(pane, narrow ? 300 : bodyHeight);
        }
        Height(NativeResults, Math.Max(200, ViewHeight(NativeScroll, 52) - 92));
        Height(DiagnosticReport, Math.Max(200, ViewHeight(DiagnosticsScroll, 52) - 58));
        Height(AssetLog, Math.Max(200, ViewHeight(AssetPageScroll) - AssetOutputFooter.ActualHeight - 110));
    }
}
