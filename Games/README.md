# Games

This is a tested subset of game config files from the [PS2 Classics Emulator Compatibility List](https://www.psdevwiki.com/ps4/Talk:PS2_Classics_Emulator_Compatibility_List).

Pull requests to update the list of game config files are welcome, however **do not** upload game .ISO files to GitHub.

## Errors

The following configuration entries cause the game to crash:

* Grand Theft Auto: Liberty City Stories (SLUS_21423)
  * `--gs-h2l-accurate-hash=1`
  * `--gs-adaptive-frameskip=1`
* Wild Arms 3
  * `--ee-static-block-links=JAL,COP2`