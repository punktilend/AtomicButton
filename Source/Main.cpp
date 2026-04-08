#include <JuceHeader.h>
#include "MainComponent.h"

class ShortcutProApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return "Shortcut Pro"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed()          override { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow ("Shortcut Pro  —  Weird Al Universe Edition",
                                          new MainComponent(),
                                          *this));
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, juce::Component* c,
                    juce::JUCEApplication& app)
            : DocumentWindow (name,
                              juce::Colour (0xff1a1c1e),
                              DocumentWindow::allButtons),
              owner (app)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);
            setResizable (true, true);
            setResizeLimits (900, 700, 1400, 1100);
            centreWithSize (960, 800);
            setVisible (true);
        }

        void closeButtonPressed() override { owner.systemRequestedQuit(); }

    private:
        juce::JUCEApplication& owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (ShortcutProApplication)
