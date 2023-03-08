#pragma once

#include <types.hpp>
#include <mast1c0re.hpp>
#include <common/list.hpp>
#include <ps/sce/usb/usb/usb.hpp>
#include <ps/sce/usb/filesystems/exfat/file.hpp>
#include <ps/sce/usb/filesystems/exfat/directory.hpp>

#define COPY_BAR_UPDATE_FREQUENCY 100

class Downloader
{
public:
    static bool download(const char* isoFilepath, const char* configFilepath, bool* hasConfig);
private:
    static bool copyUsbFileToDisk(Usb* usb, const char* filepath, exFAT::File file);
    static bool getGameToLoad(List<Usb> usbs, Usb* usb, exFAT::Directory* directory);
    static bool directoryHasFileWithExtension(Usb* usb, exFAT::Directory* directory, const char* extension);
    static void setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total);
};