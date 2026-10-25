using ModPorter.Core.Assets;

namespace ModPorter.App;
public partial class MainWindow {
    internal bool CollectionTestMode=>TestMode;
    internal Task RunCollectionOperation(string status,Func<CancellationToken,Task> work)=>RunBusy(status,work);
    internal string CollectionGamePath { get=>CurrentGamePath.Text.Trim();set=>CurrentGamePath.Text=value; }
    internal void UseCollectionOutput(DlcWorkspaceOutput output,string id){assetDirectory=output.Directory;AssetOutputLabel.Text=output.Directory;PreparedAssetPath.Text=output.DlcArchive;DlcId.Text=id;StatusText.Text=output.UpdateArchiveWritten?"Collection and first-time registration ready":"DLC collection ready · update.rpf unchanged";}
}
