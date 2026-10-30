#include "script.h"
#include "shv/xml.h"

namespace {
bool Initialize(){return scriptMemoryAvailable();}
void Stop(){(void)scriptTrimMemory();}
void Main(){
    // Complete the DOM operation before returning or yielding. These are sample
    // data values only; no game entities or world state are modified.
    bool saved=false;
    {
        pugi::xml_document document;
        const auto parsed=document.load_string("<Pose x=\"123.25\" y=\"-0.03125\"/>");
        if(parsed){
            auto pose=document.child("Pose");double x{};
            const auto input=pose.attribute("x").value();
            const bool valid=ScriptHook::Numeric::ParseDouble(input,strlen(input),x)==ScriptHook::Numeric::Status::Ok;
            if(valid&&pose.attribute("x").set_value(x+1.25)&&pose.append_child("Heading").text().set(90.125f)){
                char output[512];size_t bytes{};
                saved=ScriptHook::Xml::Save(document,output,sizeof(output),bytes)&&scriptSaveFile("pose.xml",output,bytes);
            }
        }
    }
    const char* result=saved?"saved=yes\n":"saved=no\n";
    (void)scriptSaveFile("result.txt",result,strlen(result));
}
}
SHV_NX_PLUGIN_WITH_LIFECYCLE("numeric-xml-example",Main,Initialize,Stop)
