#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"
#include <array>
#include <functional>
#include <vector>
#include <algorithm>

class KeyboardComponent : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    // Callbacks set by MainComponent
    std::function<void(int keyIndex)>                    onKeyFired;
    std::function<void(int keyIndex)>                    onKeySelected;
    std::function<void(int keyIndex, const juce::File&)> onFileDrop;
    std::function<void(int keyIndex)>                    onEditClip;

    KeyboardComponent();

    void setBank (int bankIndex, std::array<SoundSlot, NUM_KEYS>* slots);
    void setActiveKey (int keyIndex);       // highlights selected key
    void setKeyPlaying (int keyIndex, bool playing);
    void refreshKey (int keyIndex);
    void refreshAll ();
    void setUIMode (int mode);              // 0=Normal, 1=BankSelect, 2=AssignHotKey, 3=HotList
    void setHotList (const std::vector<int>& list);

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragMove  (const juce::StringArray& files, int x, int y) override;
    void fileDragExit  (const juce::StringArray& files) override;

    void paint        (juce::Graphics& g) override;
    void resized      () override;
    void mouseDown    (const juce::MouseEvent& e) override;
    void mouseUp      (const juce::MouseEvent& e) override;

private:
    // Per-key visual state
    struct KeyCell
    {
        juce::Rectangle<int> bounds;
        int   keyIndex  = -1;
        bool  isLoaded  = false;
        bool  isPlaying = false;
        bool  isSelected= false;
        bool  isDragOver= false;
    };

    std::array<KeyCell, NUM_KEYS> cells;
    int currentBank = 0;
    std::array<SoundSlot, NUM_KEYS>* currentSlots = nullptr;

    int              uiMode   = 0;
    std::vector<int> hotListKeys;

    int  dragOverKey  = -1;
    bool momentaryDown= false;
    int  momentaryKey = -1;
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    static constexpr int MARGIN  = 14;
    static constexpr int GAP     = 6;
    static constexpr int LABEL_H = 20;   // "HOT-KEYS" label at bottom

    void buildLayout();
    int  keyIndexAt (int x, int y) const;
    void paintKey   (juce::Graphics& g, const KeyCell& cell);
    void showContextMenu (int keyIndex);

    // Key display labels
    static juce::String keyLabel (int keyIndex);
    static juce::String keyDisplayChar (int keyIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardComponent)
};
