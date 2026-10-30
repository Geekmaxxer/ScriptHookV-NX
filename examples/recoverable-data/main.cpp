#include "script.h"
#include "shv/blob_store.h"
#include "shv/xml.h"

namespace {
bool Initialize() { return scriptMemoryAvailable(); }
void Stop() { (void)scriptTrimMemory(); }

bool Update() {
    using ScriptHook::Data::BlobStatus;
    ScriptHook::Data::BlobStore records(1, "counter-a.bin", "counter-b.bin");
    char input[512]{}; size_t bytes = 0;
    auto status = records.Load(input, sizeof(input) - 1, bytes);
    if (status == BlobStatus::NoValidRecord) {
        // This dedicated example explicitly initializes its own sample data.
        // Production mods must distinguish recovery policy from normal Save.
        constexpr char defaults[] = "<Counter starts=\"0\"/>";
        if (records.Initialize(defaults, sizeof(defaults) - 1) != BlobStatus::Ok)
            return false;
        status = records.Load(input, sizeof(input) - 1, bytes);
    }
    if (status != BlobStatus::Ok) return false;

    pugi::xml_document document;
    if (!document.load_buffer(input, bytes)) return false;
    auto count = document.child("Counter").attribute("starts");
    if (!count || !count.set_value(count.as_uint() + 1)) return false;
    char output[512]; size_t serialized = 0;
    if (!ScriptHook::Xml::Save(document, output, sizeof(output), serialized) ||
        records.Save(output, serialized) != BlobStatus::Ok) return false;

    size_t verified = 0;
    if (records.Load(input, sizeof(input), verified) != BlobStatus::Ok ||
        verified != serialized) return false;
    for (size_t i = 0; i < verified; ++i) if (input[i] != output[i]) return false;
    return scriptSaveFile("current.xml", input, verified);
}

void Main() {
    const bool saved = Update();
    const char* report = saved ? "saved=yes\n" : "saved=no\n";
    (void)scriptSaveFile("result.txt", report, strlen(report));
}
}
SHV_NX_PLUGIN_WITH_LIFECYCLE("recoverable-data-example", Main, Initialize, Stop)
