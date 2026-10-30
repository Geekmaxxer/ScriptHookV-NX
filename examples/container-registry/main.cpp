#include "script.h"
#include "shv/containers.h"
#include "shv/xml.h"
using namespace ScriptHook;
namespace {
struct Preset {uint32_t id{};float x{},y{},z{};Containers::String label;};
struct Registry {
    Containers::Vector<Preset> items;
    Containers::FlatMap<uint32_t,size_t> byId;
    Containers::String result;
};
Registry* registry{};
bool Initialize(){return scriptMemoryAvailable();}
void Stop(){(void)Memory::Delete(registry);(void)scriptTrimMemory();}
bool ReadFloat(pugi::xml_attribute attribute,float& value){
    const char* text=attribute.value();return attribute&&Numeric::ParseFloat(text,strlen(text),value)==Numeric::Status::Ok;
}
bool Load(){
    constexpr char source[]="<Presets><Preset id=\"1\" label=\"Harbor\" x=\"123.25\" y=\"-0.03125\" z=\"7.5\"/>"
        "<Preset id=\"2699\" label=\"Workshop\" x=\"-12.5\" y=\"1e2\" z=\"0.125\"/>"
        "<Preset id=\"42\" label=\"Hill\" x=\"0\" y=\"10.25\" z=\"256.5\"/></Presets>";
    Registry next;pugi::xml_document document;
    if(!document.load_buffer(source,sizeof(source)-1))return false;
    for(auto node=document.child("Presets").child("Preset");node;node=node.next_sibling("Preset")){
        Preset preset;preset.id=node.attribute("id").as_uint();
        if(!preset.label.TryAssign(node.attribute("label").value())||!ReadFloat(node.attribute("x"),preset.x)||!ReadFloat(node.attribute("y"),preset.y)||!ReadFloat(node.attribute("z"),preset.z))return false;
        if(next.byId.TryInsert(preset.id,next.items.Size())!=Containers::InsertResult::Inserted||!next.items.TryPushBack(std::move(preset)))return false;
    }
    const auto* index=next.byId.Find(2699);if(!index||next.items.At(*index)->x!=-12.5f)return false;
    if(!next.result.TryAssign("loaded=yes\nrecords=3\n"))return false;
    *registry=std::move(next);return true; // Only commit the complete in-memory registry.
}
bool Save(){
    pugi::xml_document document;auto root=document.append_child("Presets");if(!root)return false;
    for(const auto& preset:registry->items){
        auto node=root.append_child("Preset");
        if(!node||!node.append_attribute("id").set_value(preset.id)||!node.append_attribute("label").set_value(preset.label.CStr())||!node.append_attribute("x").set_value(preset.x)||!node.append_attribute("y").set_value(preset.y)||!node.append_attribute("z").set_value(preset.z))return false;
    }
    char output[2048];size_t bytes{};
    return Xml::Save(document,output,sizeof(output),bytes)&&scriptSaveFile("presets.xml",output,bytes);
}
void Main(){
    if(!registry)registry=Memory::TryNew<Registry>();
    if(registry&&Load()&&Save())(void)scriptSaveFile("result.txt",registry->result.CStr(),registry->result.Size());
    else (void)scriptSaveFile("result.txt","loaded=no\n",10);
    // Return only after local DOMs have been destroyed. Runtime cleanup then
    // deletes module-owned containers and trims unused memory.
}
}
SHV_NX_PLUGIN_WITH_LIFECYCLE("container-registry-example",Main,Initialize,Stop)
