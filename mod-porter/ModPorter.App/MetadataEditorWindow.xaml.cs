using ModPorter.Core.Assets;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;

namespace ModPorter.App;
public partial class MetadataEditorWindow : Window {
    readonly DlcWorkspace workspace;
    bool loading,dirty,busy;
    string? path;
    public MetadataEditorWindow(DlcWorkspace workspace,string? selected){
        InitializeComponent();this.workspace=workspace;MetadataFile.ItemsSource=workspace.MetadataFiles();MetadataFile.SelectedItem=selected;if(MetadataFile.SelectedItem==null)MetadataFile.SelectedIndex=0;
    }
    private void Xml_Changed(object sender,TextChangedEventArgs e){if(!loading)dirty=true;}
    private void File_Changed(object sender,SelectionChangedEventArgs e){
        if(loading||MetadataFile.SelectedItem is not string selected)return;
        if(dirty&&MessageBox.Show(this,"Discard unapplied changes to this metadata file?","Metadata editor",MessageBoxButton.YesNo,MessageBoxImage.Question,MessageBoxResult.No)!=MessageBoxResult.Yes){loading=true;MetadataFile.SelectedItem=path;loading=false;return;}
        loading=true;try{path=selected;XmlText.Text=workspace.ReadMetadata(path);dirty=false;}finally{loading=false;}
    }
    internal async Task ApplyAsync(){
        if(path==null||busy)return;busy=true;ApplyButton.IsEnabled=false;MetadataFile.IsEnabled=false;XmlText.IsReadOnly=true;string value=XmlText.Text,selected=path;
        try{await Task.Run(()=>workspace.SetMetadata(selected,value));dirty=false;Status.Text="Metadata applied to working collection · Save DLC to export.";}
        catch(Exception error)when(error is not OutOfMemoryException){Status.Text=error.Message;}
        finally{busy=false;ApplyButton.IsEnabled=true;MetadataFile.IsEnabled=true;XmlText.IsReadOnly=false;}
    }
    private async void Apply_Click(object sender,RoutedEventArgs e)=>await ApplyAsync();
    private void Close_Click(object sender,RoutedEventArgs e)=>Close();
    private void Window_Closing(object? sender,CancelEventArgs e){if(busy){e.Cancel=true;return;}if(dirty&&MessageBox.Show(this,"Discard unapplied XML changes?","Metadata editor",MessageBoxButton.YesNo,MessageBoxImage.Question,MessageBoxResult.No)!=MessageBoxResult.Yes)e.Cancel=true;}
}
