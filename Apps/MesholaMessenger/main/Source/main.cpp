#include "MesholaApp.h"
#include <Tactility/app/AppRegistration.h>
#include <lvgl.h>

namespace meshola {

const tt::app::AppManifest manifest = {
    .targetSdk = "0.7.0",
    .targetPlatforms = "esp32s3",
    .appId = "com.meshola.messenger",
    .appName = "Meshola Messenger",
    .appIcon = "*",
    .appCategory = tt::app::Category::User,
    .createApp = tt::app::create<MesholaApp>
};

} // namespace meshola

#ifndef MESHOLA_MESSENGER_EMBED
extern "C" void app_main(void) {
    extern void compat_force_link(void);
    compat_force_link(); // ensure compat stubs are linked for app ELF packaging

    tt::app::addAppManifest(meshola::manifest);
}
#endif
