#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

// ── Constants ─────────────────────────────────────────────
static constexpr int   NUM_BANKS   = 4;
static constexpr int   NUM_KEYS    = 47;   // trigger keys per bank
static constexpr int   MAX_VOICES  = 64;   // simultaneous voices

// The 47 trigger keys in keyboard order (using char representation)
// Space stored as 0x20
static constexpr char TRIGGER_KEYS[] = {
    '`','1','2','3','4','5','6','7','8','9','0','-','=',   // row 0 — 13
    'q','w','e','r','t','y','u','i','o','p','[',']',       // row 1 — 12
    'a','s','d','f','g','h','j','k','l',';','\'',          // row 2 — 11
    'z','x','c','v','b','n','m',',','.',  '/',             // row 3 — 10
    ' '                                                     // space  — 1
};

static constexpr int TRIGGER_KEY_COUNT = sizeof(TRIGGER_KEYS) / sizeof(char);

// ── Preset names [bank 0-3][key index 0-46] ──────────────
// Banks: A=0 UHF/VHS, B=1 Song hooks, C=2 Deep cuts, D=3 Specialty
static const char* PRESET_NAMES[4][47] = {
    // Bank A — UHF / VHS
    {
        "UHF STING","NO STINK BADGER","SUPPLIES!","SPATULA CITY","CONAN LIBRARIAN",
        "WHEEL O FISH","GANDHI II","SPADOWSKI","BEVERAGE CART","YOU SO STUPID",
        "UHF THEME","RAUL INTRO","TOWN TALK",
        "QUICK CHANGE","WILD KINGDOM","EVEL K JR","RAUL'S SHOW","TOOTHPICK TRK",
        "YODA IMPRSN","UNCLE NUTSY","I LUV PLACE","ONE DOLLAR!","PAULY'S DEBUT",
        "POOL TRICK","PUPPET SHOW",
        "AL'S SPEECH","SPATULA PITCH","DOROTHY SHOES","FLYING GANDHI","GANDHI TRLR",
        "HOTEL LOBBY","JANITOR PALCE","KIWI SLAM","LARRY LAUNDRY","SQUEAK FX",
        "BADGER QUOTE",
        "ZERO DOLLARS","XMAS VHS","COLONIC IRGTN","VHS TRACKING","BEV SMASH",
        "NOOGIE NUTSY","MY SPLEEN!","SLIGHT RETURN","THATS ALL","FADE OUT",
        "BIG FIRE"
    },
    // Bank B — Classic songs
    {
        "ACCORDION RFF","EAT IT HOOK","FAT HOOK","AMISH PARADSE","LIKE A SURGN",
        "WHITE&NERDY","SMELLS NIRVNA","JURASSIC PRK","BEDROCK ANTM","ALBUQUERQUE",
        "HARDWARE STOR","POLKA PARTY","DARE STUPID",
        "QUEEN SUEDE","WORD CRIMES","EAT IT VERSE","RUN W SCISSRS","TACKY HOOK",
        "YODA SONG","UGLY GIRL","ILL SUE YA","ODE FAMILY","PRFRM THIS WY",
        "POLKA FACE","PRINCESS BRIDE",
        "ANOTHER RIDES","SAGA BEGINS","DONT DOWNLOAD","FIRST WLD PRBS","GENERIC BLUES",
        "HEADLINE NEWS","JACKSON PARK","KING OF SUEDE","LASAGNA","SRGN FX",
        "AL QUOTE",
        "ZERO HOUR","X AMISH","CNR HOOK","VIRUS ALERT","BOHEMIAN LIKE",
        "NATURE TRAIL","MY BOLOGNA","COMMA KARMA","POINT BREAK","JUST EAT IT",
        "ACCORDION STAB"
    },
    // Bank C — Al TV / Deep cuts / Interview
    {
        "RADIO CALL IN","AL TV MJ","AL TV MADONNA","AL TV DIRE ST","AL TV COOLIO",
        "AL TV HAMMER","AL TV NIRVANA","AL TV LYNYRD","AL TV AEROSMTH","AL TV VAN HLN",
        "AL TV MEATLOAF","BEHIND SCENES","VLOG INTRO",
        "QUESTION AL","WENDY INTRVIEW","EARLY KIDSHOW","RUNNING GAGS","TOUR DIARY",
        "YEARBOOK PIC","UNDER PRESSURE","IN 3D SHOW","OPEN CHEESE","POODLE BONUS",
        "POLKA FACE BNS","POLKA FACE 2",
        "AL FAN MAIL","SCOTTI BROS","DIRECT HELL","FAN LTR READ","GRAMMY SPEECH",
        "HIT ME BABY","JURASSIC FX","KARAOKE SCENE","LIVE KNOTTS","SATURDAY NGT",
        "TOUR BLOOPER",
        "ZIP IT GOOD","XYLOPHONE GAG","CLOSE ENCOUNT","VHS STATIC","BONUS FEATURE",
        "NETWORK CLIP","MY LIFE","LITTLE MOMENT","PERIOD CORCT","FLIP SIDE",
        "CROWD NOISE"
    },
    // Bank D — Specialty / rarities
    {
        "WEASEL STOMP","FOIL INTRO","TACKY VERSE","MISSION STMT","SPORTS SONG",
        "EBAY HOOK","VIRUS FX","CNR VERSE","WORD CRIMES V","1ST WLD PRBS",
        "JACKSON PRK E","RINGTONE PRD","VELVET ELVIS",
        "QUEEN SUEDE V","WEASEL STOMP V","EVERYTHING U","RICKY HOOK","TRPD DRV-THRU",
        "U DON'T LUV ME","UNDEF HERO","STILL BILL","OPEN MIC NGT","PARTY GROUND",
        "LAME CLAIM FM","JUST A JOKE",
        "AMISH PARDSE V","SMELLS VERSE","DONT WR WHITE","FRANK 2000 TV","GENIUS FRANCE",
        "HANDY HOOK","JACKSON ALT","KILLIN TIME","SURGN LIVE","SPORTS REF",
        "OTHER DEEP CUT",
        "ZELDA POLKA","XMAS AT GRND","CAVITY SEARCH","VELVET VERSE","BOB DYLAN STL",
        "NATURE TRAIL V","MIDNIGHT STAR","AL MINUTE","ALBUQ END","HARDWARE F",
        "ACCORDION CHRD"
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

    // Waveform thumbnail data (normalised peaks, display resolution)
    std::vector<float> waveformPeaks;

    // Per-slot parameters
    float gain     = 1.0f;   // linear
    float trimIn   = 0.0f;   // fraction 0-1
    float trimOut  = 1.0f;   // fraction 0-1

    bool  isLoaded() const noexcept { return buffer != nullptr; }

    int64_t trimInSample()  const noexcept {
        return isLoaded() ? static_cast<int64_t>(trimIn  * buffer->getNumSamples()) : 0;
    }
    int64_t trimOutSample() const noexcept {
        return isLoaded() ? static_cast<int64_t>(trimOut * buffer->getNumSamples()) : buffer->getNumSamples();
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
    bool    active     = false;
    int     keyIndex   = -1;   // which slot owns this voice
    int     bankIndex  = -1;
};
