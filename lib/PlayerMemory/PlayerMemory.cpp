#include "PlayerMemory.h"
#include "Player.h"

PlayerMemory::SourceMemory PlayerMemory::forSource(SourceType type) {
    switch (type) {
    case SourceType::Flash:  return {&dirFlash, &fileFlash, &songFlash};
    case SourceType::SdCard: return {&dirSd, &fileSd, &songSd};
    case SourceType::DeepSid: return {&dirDeepSid, &fileDeepSid, &songDeepSid};
    default:                 return {nullptr, nullptr, nullptr};
    }
}

void PlayerMemory::apply(Player& player) {
    player.setVolume(volume);
    player.setMute(muted);
    player.setModel(sidModel);
    for (uint8_t i = 0; i < SOURCE_TYPE_COUNT; i++) {
        SourceMemory m = forSource(SourceType(i));
        if (m.dir) player.setDir(SourceType(i), *m.dir);
    }
    // niewkompilowane zrodlo (np. program bez karty SD) - zostaje pamiec flash
    if (player.setSource(SourceType(source)).length() && !sourceInfo(SourceType(source)))
        player.setSource(SourceType::Flash);
}

void PlayerMemory::capture(Player& player) {
    PlayerStatus s = player.status();
    volume = s.volume;
    muted = s.muted;
    sidModel = s.modelSetting;
    source = uint8_t(s.source);
    for (uint8_t i = 0; i < SOURCE_TYPE_COUNT; i++) {
        SourceMemory m = forSource(SourceType(i));
        if (m.dir) *m.dir = player.dir(SourceType(i));
    }
    if (s.loaded) {
        SourceMemory m = forSource(s.fileSource);
        if (m.file) { *m.file = s.file; *m.song = s.song; }
    }
}

String PlayerMemory::lastFile(Player& player) {
    SourceMemory m = forSource(player.source());
    return m.file ? *m.file : String();
}

uint8_t PlayerMemory::lastSong(Player& player) {
    SourceMemory m = forSource(player.source());
    return m.song ? *m.song : 0;
}
