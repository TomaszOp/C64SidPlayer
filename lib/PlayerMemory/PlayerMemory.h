/**
 * @file PlayerMemory.h
 * @brief PlayerMemory - player settings stored in NVS (NvsSettings mechanism).
 *
 * The list of settings is in PlayerSettings.def. Usage:
 * @code
 *   PlayerMemory mem;
 *   mem.load();                 // in setup(): read, nothing stored = default values
 *   mem.apply(player);          // volume, SID model, source and folders -> player
 *   mem.capture(player);        // current player state -> settings
 *   mem.save();                 // writes only what has changed
 *   mem.setDefault(true);       // default values and clearing NVS
 * @endcode
 */
#pragma once
#include <NvsSettings.h>
#include <SidSource.h>

#define NVS_CLASS     PlayerSettings
#define NVS_DEF       "PlayerSettings.def"
#define NVS_NAMESPACE "c64sid"
#define NVS_VERSION   1
#include <NvsSettingsClass.h>

class Player;

/**
 * @brief Player settings (PlayerSettings.def) plus transferring them to / from a Player.
 */
class PlayerMemory : public PlayerSettings {
public:
    /** @brief Fields remembered for one source (nullptr = no fields for this source). */
    struct SourceMemory { String* dir; String* file; uint8_t* song; };
    /**
     * @brief Returns pointers to the folder / file / subtune fields of a source.
     * @param type Source.
     * @return Field pointers (all nullptr for an unknown source).
     */
    SourceMemory forSource(SourceType type);

    /**
     * @brief Applies the settings to the player: volume, mute, SID model, folders and the active source.
     *
     * A source that is not compiled in falls back to flash memory.
     * @param player Player.
     */
    void apply(Player& player);
    /**
     * @brief Copies the current player state into the settings (does not save).
     * @param player Player.
     */
    void capture(Player& player);
    /**
     * @brief Last file of the active source - to play after power-on.
     * @param player Player (for the active source).
     * @return Full path, "" when none.
     */
    String lastFile(Player& player);
    /**
     * @brief Last subtune of the active source.
     * @param player Player (for the active source).
     * @return Subtune number, 0 = default.
     */
    uint8_t lastSong(Player& player);
};
