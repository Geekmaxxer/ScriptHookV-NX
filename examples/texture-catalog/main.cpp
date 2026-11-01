#include "script.h"
#include "shv/text.h"

namespace {
constexpr unsigned TextureCount = 80;
int textures[TextureCount];
unsigned run, loaded, failed, frames, stop_count;
bool duplicates_verified, retained_verified;

void Filename(unsigned index, char (&name)[11]) {
    const char pattern[] = "tile00.png";
    for (unsigned i = 0; i < sizeof(pattern); ++i) name[i] = pattern[i];
    name[4] = char('0' + index / 10);
    name[5] = char('0' + index % 10);
}

void Report() {
    ScriptHook::Util::TextBuffer<384> report;
    report.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\nloaded=").UInt(loaded).Add("\ncompleted_frames=").UInt(frames)
        .Add("\nduplicate_ids_verified=").UInt(duplicates_verified)
        .Add("\nretained_ids_verified=").UInt(retained_verified)
        .Add("\nstop_count=").UInt(stop_count).Add("\n");
    if (report.Complete()) (void)scriptSaveFile("result.txt", report.CStr(), report.Size());
}

void Main() {
    ++run;
    loaded = failed = frames = 0;
    duplicates_verified = retained_verified = true;
    if (!(getScriptHookCapabilities() & SHV_NX_CAP_CUSTOM_TEXTURES)) {
        failed = 1;
        Report();
        return;
    }
    for (unsigned i = 0; i < TextureCount; ++i) {
        char name[11]{};
        Filename(i, name);
        const int previous = textures[i];
        const int texture = createTexture(name);
        if (!texture) { ++failed; break; }
        if (createTexture(name) != texture) duplicates_verified = false;
        if (previous && previous != texture) retained_verified = false;
        textures[i] = texture;
        ++loaded;
        WAIT(0);
    }
    if (loaded == TextureCount) {
        for (; frames < 360; ++frames) {
            const unsigned first = ((frames / 36) % 5) * 16;
            for (unsigned cell = 0; cell < 16; ++cell) {
                const ShvNxTextureDraw draw = {
                    textures[first + cell], 0, int32_t(cell), 0,
                    .065f, .065f, .5f, .5f,
                    .36f + float(cell % 4) * .09f, .28f + float(cell / 4) * .12f,
                    float(frames % 120) / 120.f, 1.f, 1.f, 1.f, 1.f, .75f
                };
                if (!drawTextureChecked(&draw)) ++failed;
            }
            WAIT(0);
        }
    }
    if (!duplicates_verified || !retained_verified) ++failed;
    Report();
}

void Stop() {
    ++stop_count;
    Report();
}
}

SHV_NX_PLUGIN_WITH_STOP("texture-catalog-example", Main, Stop)
