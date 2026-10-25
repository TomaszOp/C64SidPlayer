#include "PlayerMemory.h"
#include "Player.h"

void PlayerMemory::apply(Player& player) const {
    player.setVolume(volume);
    player.setMute(muted);
    player.setModel(sidModel);
}

void PlayerMemory::capture(Player& player) {
    PlayerStatus s = player.status();
    volume = s.volume;
    muted = s.muted;
    sidModel = s.modelSetting;
    if (s.loaded) {
        lastFile = s.file;
        lastSong = s.song;
    }
}
