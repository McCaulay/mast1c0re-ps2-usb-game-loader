#include <mast1c0re.hpp>
#include "downloader/Downloader.hpp"

char* getFilepathBypass(char* dest, const char* src);

void main()
{
    // Set pad light to purple
    PS::PadSetLightBar(150, 0, 255, 255);

    // Show "PS2 Game Loader" notification
    PS::notificationWithIcon("cxml://psnotification/tex_morpheus_trophy_platinum", "PS2 Game Loader (USB)");

    // Attempt to connect to debug server
    // PS::Debug.connect(IP(192, 168, 0, 7), 9023);
    PS::Debug.printf("---------- Load PS2 Game (USB) ----------\n");

    // Filepaths
    const char* isoFilepath = "/av_contents/content_tmp/disc01.iso";
    const char* configFilepath = "/av_contents/content_tmp/SCUS-97129_cli.conf";

    // Download ISO from USB
    bool hasConfig = false;
    if (!Downloader::download(isoFilepath, configFilepath, &hasConfig))
    {
        // Disconnect from debug server
        PS::Debug.disconnect();
        return;
    }

    char relativeIsoFilepath[512];
    char relativeConfigFilepath[512];

    // Mount & Load iso
    PS::Debug.printf("Mounting %s\n", isoFilepath);
    PS::MountDiscWithFilepath(getFilepathBypass(relativeIsoFilepath, isoFilepath));

    // Get game code from mounted file
    char* gameCode = PS::GetMountedGameCode();
    if (PS2::strlen(gameCode) == 10)
    {
        // Convert name from "SCUS-97129" -> "cdrom0:\\SCUS_971.29;1"
        char* ps2Path = PS2::gameCodeToPath(gameCode);

        // Load configuration file
        if (hasConfig)
        {
            PS::Debug.printf("Processing config %s\n", configFilepath);
            PS::ProcessConfigFile(getFilepathBypass(relativeConfigFilepath, configFilepath));
        }

        // Disconnect from debug server
        PS::Debug.printf("Loading \"%s\"...\n", ps2Path);

        // Disconnect from debug server
        PS::Debug.disconnect();

        // Restore corruption
        PS::Breakout::restore();

        // Execute mounted iso
        PS2::LoadExecPS2(ps2Path, 0, NULL);
        return;
    }

    PS::notification("Unexpected game code \"%s\"!", gameCode);
    PS::Debug.printf("Unexpected game code (%s) length of %i, expecting %i\n", gameCode, PS2::strlen(gameCode), 10);

    // Disconnect from debug server
    PS::Debug.disconnect();
}

// "/av_contents/content_tmp/disc01.iso" -> "./../av_contents/content_tmp/disc01.iso"
// "/av_contents/content_tmp/SCUS-97129_cli.conf" -> "./../av_contents/content_tmp/SCUS-97129_cli.conf"
char* getFilepathBypass(char* dest, const char* src)
{
    uint32_t offset = 0;

    // Prefix "./.."
    PS2::strcpy(dest, "./..");
    offset += 4;

    // Append "/"
    if (src[0] != '/')
    {
        dest[offset] = '/';
        offset++;
    }

    // Copy source
    PS2::strcpy(dest + offset, src);
    offset += PS2::strlen(src);

    // NULL terminator
    dest[offset] = '\0';
    return dest;
}