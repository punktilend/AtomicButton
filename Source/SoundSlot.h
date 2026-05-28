#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

// ── Constants ─────────────────────────────────────────────
static constexpr int   NUM_KEYS    = 50;   // trigger keys per bank (IR2: 10x5 grid)
static constexpr int   MAX_VOICES  = 64;   // simultaneous voices

// The 50 trigger keys in IR2 grid order (row by row, 10 per row)
// '\0' = blank/non-triggerable position
static constexpr char TRIGGER_KEYS[50] = {
    // Row 1: 1-10
    '1','2','3','4','5','6','7','8','9','0',
    // Row 2: 11-20
    'q','w','e','r','t','y','u','i','o','p',
    // Row 3: 21-30
    'a','s','d','f','g','h','j','k','l','\0',
    // Row 4: 31-40
    '\0','z','x','c','v','b','n','m','-','=',
    // Row 5: 41-50
    '\0','\0','\0','\0',' ','\0','\0',',','.','/'
};

// ── Preset names [bank 0-3][key index 0-49] ──────────────
// Banks: A=0 Imaging, B=1 Beds/Hooks, C=2 Interview/Actuality, D=3 Specialty/FX
static const char* PRESET_NAMES[4][50] = {
    // Bank A — Imaging / opens / IDs
    {
        "OPEN STING","SHOW OPEN","NEWS OPEN","SPORTS OPEN","WEATHER OPEN",
        "LEGAL ID","TOP HOUR","REJOIN","COLD OPEN","BREAK STING",
        "THEME HIT","INTRO BED","OUTRO TAG",
        "MORNING ID","MIDDAY ID","DRIVE ID","NIGHT ID","WEEKEND ID",
        "LIVE READ","STATION VOX","QUICK DROP","LINER ONE","LINER TWO",
        "PROMO TAG","CONTEST TAG",
        "NEWSER BED","SPORTS BED","WX BED","TRAFFIC BED","TALK BED",
        "SPONSOR TAG","CALLER UP","PHONE BED","CROWD LIFT","CHEER HIT",
        "AUDIENCE FX",
        "BREAK ONE","BREAK TWO","BREAK THREE","RESET SWEEP","HOUR MARK",
        "BUMPER ONE","BUMPER TWO","END TAG","FADE OUT","HARD OUT",
        "BIG FINISH","SLOT 48","SLOT 49","SLOT 50"
    },
    // Bank B — Beds / hooks / promos
    {
        "HOOK ONE","HOOK TWO","HOOK THREE","POWER INTRO","PROMO OPEN",
        "ALT BED","ROCK BED","POP BED","DANCE BED","HIPHOP BED",
        "RHYTHM BED","PUNCH BED","COUNTDOWN",
        "FEATURE OPEN","FEATURE TAG","MIX HIT","DROP FX","SWOOSH FX",
        "RAMP ONE","RAMP TWO","RAMP THREE","MUSIC BED A","MUSIC BED B",
        "PROMO ONE","PROMO TWO",
        "TEASE ONE","TEASE TWO","TEASE THREE","COMEBACK","POWER STAB",
        "HEADLINE BED","UPDATE BED","COUNTDOWN FX","RISE FX","HIT SWEEP",
        "VOICE DROP",
        "TOPICAL","RETRO BED","WEEKEND BED","MIX SHOW","PARTY BED",
        "NIGHT BED","LATE BED","COMEDOWN","POINTER","HARD TEASE",
        "STAB FX","SLOT 48","SLOT 49","SLOT 50"
    },
    // Bank C — Interview / actuality / production
    {
        "CALLER ONE","CALLER TWO","CALLER THREE","VOX POP ONE","VOX POP TWO",
        "ACTUALITY A","ACTUALITY B","ACTUALITY C","INTERVIEW A","INTERVIEW B",
        "INTERVIEW C","BEHIND SCENE","FIELD OPEN",
        "QUESTION ONE","QUESTION TWO","ANSWER ONE","ANSWER TWO","REMOTE BED",
        "PKG INTRO","PKG OUT","NEWS ACT","CLIP ONE","CLIP TWO",
        "CLIP THREE","CLIP FOUR",
        "MAILBAG","TAPE ROLL","LONGFORM","FEATURE PKG","AWARD CLIP",
        "PHONE HOOK","ROOM TONE","SCENE SET","LIVE SHOT","SAT NAT",
        "BLOOPER",
        "ZIPPER","XYLO FX","WHOOSH","TAPE HISS","BONUS CUT",
        "NETWORK FEED","LIFE BED","MOMENT","PERIODIC","FLIPPER",
        "CROWD FX","SLOT 48","SLOT 49","SLOT 50"
    },
    // Bank D — Specialty / comedy / utility / FX
    {
        "COMEDY HIT","PRANK OPEN","PRANK BED","MISSION OPEN","SPORTS FX",
        "SALE TAG","ALARM FX","CLUB BED","GLITCH FX","ODDBALL",
        "PARKER","RINGTONE","NOVELTY",
        "VARIATION A","VARIATION B","UTILITY ONE","UTILITY TWO","DRIVETHRU",
        "HEARTBED","HERO BED","STILLER","OPEN MIC","PARTY FX",
        "CLAIM GAME","JOKE TAG",
        "VARIANT C","VERSE BED","WHITE NOISE","RETRO TV","GAME SHOW",
        "HAND TOOL","ALT TAKE","KILL TIME","LIVE BED","SPORT REF",
        "DEEP CUT",
        "ARCADE FX","HOLIDAY","SEARCH FX","VELVET BED","FOLK BED",
        "TRAILER","MIDNIGHT","ONE MINUTE","ENDING","HARDWARE",
        "CHORD HIT","SLOT 48","SLOT 49","SLOT 50"
    }
};

// ── SoundSlot ─────────────────────────────────────────────
struct SoundSlot
{
    juce::String  name;
    int           bankIndex  = 0;
    int           keyIndex   = 0;
    char          key        = 0;

    // Decoded audio — loaded once, read many times
    std::shared_ptr<juce::AudioBuffer<float>> buffer;
    int    sampleRate  = 44100;
    double duration    = 0.0;  // seconds
    juce::File sourceFile;      // which file is loaded

    // Waveform thumbnail data (normalised peaks, display resolution)
    std::vector<float> waveformPeaks;

    // Per-slot parameters
    float gain     = 1.0f;   // linear
    float trimIn   = 0.0f;   // fraction 0-1
    float trimOut  = 1.0f;   // fraction 0-1
    float fadeIn   = 0.0f;   // fade-in duration in seconds
    float fadeOut  = 0.0f;   // fade-out duration in seconds

    bool  isLoaded() const noexcept { return buffer != nullptr; }

    int64_t trimInSample()  const noexcept {
        return isLoaded() ? static_cast<int64_t>(trimIn  * buffer->getNumSamples()) : 0;
    }
    int64_t trimOutSample() const noexcept {
        return isLoaded() ? static_cast<int64_t>(trimOut * buffer->getNumSamples()) : 0;
    }
};

// ── Voice (one live playback instance) ───────────────────
struct Voice
{
    std::shared_ptr<juce::AudioBuffer<float>> buffer;
    int64_t readPos    = 0;
    int64_t startSamp  = 0;
    int64_t endSamp    = 0;
    float   gain       = 1.0f;
    bool    loop       = false;
    int64_t fadeInSamps  = 0;   // precomputed from fadeIn * sampleRate
    int64_t fadeOutSamps = 0;   // precomputed from fadeOut * sampleRate
    bool    active     = false;
    bool    paused     = false;
    int     keyIndex   = -1;   // which slot owns this voice
    int     bankIndex  = -1;
};
