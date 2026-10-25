// Generator klasy ustawien - zobacz opis w NvsSettings.h.
// Przed dolaczeniem zdefiniuj NVS_CLASS, NVS_DEF, NVS_NAMESPACE i NVS_VERSION.
// (bez #pragma once - plik jest dolaczany raz dla kazdego zestawu ustawien)
#include "NvsSettings.h"

#if !defined(NVS_CLASS) || !defined(NVS_DEF) || !defined(NVS_NAMESPACE) || !defined(NVS_VERSION)
#error "Zdefiniuj NVS_CLASS, NVS_DEF, NVS_NAMESPACE i NVS_VERSION przed #include <NvsSettingsClass.h>"
#endif

class NVS_CLASS : public NvsSettings {
public:
#define SETTING(type, name, key, def) type name = def;
#include NVS_DEF
#undef SETTING

    explicit NVS_CLASS(const char* nvsNamespace = NVS_NAMESPACE) : NvsSettings(nvsNamespace, NVS_VERSION) {}

protected:
    void visit(Visitor& v) override {
#define SETTING(type, name, key, def) { static const type d_ = def; v.field(key, #name, name, d_); }
#include NVS_DEF
#undef SETTING
    }
};

#undef NVS_CLASS
#undef NVS_DEF
#undef NVS_NAMESPACE
#undef NVS_VERSION
