#include "Downloader.hpp"

bool Downloader::download(const char* isoFilepath, const char* configFilepath, bool* hasConfig)
{
    bool downloaded = true;
    bool copiedIso = false;
    *hasConfig = false;

    // Get a list of connected USBs
    List<Usb> usbs = Usb::list();
    
    // Get game directory to load
    Usb usb;
    exFAT::Directory directory;
    if (Downloader::getGameToLoad(usbs, &usb, &directory))
    {
        List<exFAT::File> files = usb.files(directory);
        for (uint32_t i = 0; i < files.size(); i++)
        {
            PS::Debug.printf("> Game File: %s/%s\n", directory.getName(), files[i].getName());

            // Copy .iso
            if (!copiedIso && files[i].hasExtension("iso"))
            {
                downloaded &= Downloader::copyUsbFileToDisk(&usb, isoFilepath, files[i]);
                copiedIso = true;
                continue;
            }

            // Copy .conf
            if (!(*hasConfig) && files[i].hasExtension("conf"))
            {
                downloaded &= Downloader::copyUsbFileToDisk(&usb, configFilepath, files[i]);
                *hasConfig = true;
                continue;
            }
        }
    }
    else
        downloaded = false;

    // Cleanup
    for (uint32_t i = 0; i < usbs.size(); i++)
        usbs[i].unmount();
    usbs.free();

    return downloaded && copiedIso;
}

bool Downloader::copyUsbFileToDisk(Usb* usb, const char* filepath, exFAT::File file)
{
    uint64_t filesize = file.getSize();
    if (filesize == 0 || !usb->resetRead(file))
    {
        PS::notification("Error: %s is empty", file.getName());
        PS::Debug.printf("Error: %s is empty\n", file.getName());
        return false;
    }

    // Write file to device
    PS::Debug.printf("Writing %s (%lu) to %s\n", file.getName(), filesize, filepath);
    int fd = PS::open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd <= 0)
    {
        PS::notification("Error: Failed to open %s", filepath);
        PS::Debug.printf("Error: Failed to open %s\n", filepath);
        return false;
    }

    bool errorOccured = false;

    char dialogMessage[512];
    PS2::memset(dialogMessage, 0, sizeof(dialogMessage));
    PS2::sprintf(dialogMessage, "Loading %s...", file.getName());

    PS::Sce::MsgDialog::Initialize();
    PS::Sce::MsgDialogProgressBar progressDialog = PS::Sce::MsgDialogProgressBar(dialogMessage);
    progressDialog.open();
    Downloader::setProgress(progressDialog, 0, filesize);

    // Write to HDD in chunks
    uint32_t clusterSize = usb->getClusterSize();
    uint8_t* buffer = (uint8_t*)PS2::malloc(clusterSize);
    uint32_t updateBar = 0;
    for (uint64_t i = 0; i < filesize; i += clusterSize)
    {
        // Get remainder
        uint32_t copySize = clusterSize;
        if (i + copySize > filesize)
            copySize = filesize % copySize;

        // Read cluster from USB
        if (!usb->readNextCluster(file, buffer))
        {
            PS::notification("Error: Failed to read file from USB!");
            PS::Debug.printf("Failed to read file at offset %llu from USB\n", i);
            errorOccured = true;
            break;
        }

        // Write to file
        size_t writeCount = PS::writeAll(fd, buffer, copySize);
        if (writeCount != copySize)
        {
            PS::notification("Error: Failed to write file to disk!");
            PS::Debug.printf("Failed to write file Games/%s, wrote %llu, expected to write %llu\n", file.getName(), writeCount, copySize);
            errorOccured = true;
            break;
        }

        // Update progress bar
        if (updateBar == COPY_BAR_UPDATE_FREQUENCY)
        {
            Downloader::setProgress(progressDialog, i, filesize);
            updateBar = 0;
        }
        updateBar++;
    }
    PS::close(fd);
    PS2::free((void*)buffer);

    progressDialog.setValue(100);
    progressDialog.close();
    PS::Sce::MsgDialog::Terminate();

    return !errorOccured;
}

bool Downloader::getGameToLoad(List<Usb> usbs, Usb* usb, exFAT::Directory* directory)
{
    if (usbs.size() == 0)
    {
        PS::notification("Error: No USB found");
        return false;
    }

    // Check each USB
    bool gamesFound = false;
    for (uint32_t i = 0; i < usbs.size(); i++)
    {
        // Open USB
        if (!usbs[i].open())
        {
            PS::Debug.printf("Error: Failed to open USB #%i\n", i + 1);
            continue;
        }

        // Mount USB
        if (!usbs[i].mount())
        {
            PS::Debug.printf("Error: Failed to mount USB #%i\n", i + 1);
            continue;
        }

        // Check "Games" directory exists
        PS::Debug.printf("Checking games...\n");
        if (!usbs[i].exists("/Games/"))
        {
            PS::Debug.printf("Error: Directory \"/Games/\" does not exist on USB #%i\n", i + 1);
            continue;
        }

        // Loop each directory in games directory
        List<exFAT::Directory> directories = usbs[i].directories("/Games");
        PS::Debug.printf("Found %i directories in games directory\n", directories.size());

        for (uint32_t j = 0; j < directories.size(); j++)
        {
            // Ignore directories not containing an iso
            if (!Downloader::directoryHasFileWithExtension(&(usbs[i]), &directories[j], "iso"))
                continue;

            gamesFound = true;
            PS::Debug.printf("Found game: %s\n", directories[j].getName());

            char message[512];
            PS2::sprintf(message, "Do you want to play %s?", directories[j].getName());
            if (PS::Sce::MsgDialogUserMessage::show(message, PS::Sce::MsgDialog::ButtonType::YESNO))
            {
                *usb = usbs[i];
                *directory = directories[j];
                return true;
            }
        }
    }

    PS::notification("Error: No game %s\n", gamesFound ? "selected" : "found");
    return false;
}

bool Downloader::directoryHasFileWithExtension(Usb* usb, exFAT::Directory* directory, const char* extension)
{
    List<exFAT::File> files = usb->files(*directory);
    for (uint32_t i = 0; i < files.size(); i++)
    {
        if (files[i].hasExtension(extension))
            return true;
    }
    return false;
}

void Downloader::setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total)
{
    if (total == 0)
        return;

    // Calculate percentage without float/double
    uint64_t divident = downloaded * 100;
    uint64_t percentage = 0;

    while (divident >= total)
    {
        divident -= total;
        percentage++;
    }

    dialog.setValue(percentage);
}