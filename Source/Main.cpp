#include <JuceHeader.h>
#include "MainComponent.h"

class ShortcutProApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return "Atomic Button"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed()          override { return false; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow ("Atomic Button  -  Broadcast Audio Editor",
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
            setContentOwned (c, false);
            setResizable (true, true);
            setResizeLimits (900, 600, 3840, 2160);

            // Size to the actual usable display area (logical units) so the
            // deck always fits regardless of monitor size / DPI scaling.
            auto ua = juce::Desktop::getInstance().getDisplays()
                          .getPrimaryDisplay()->userArea;
            const int w = juce::jmin (1480, (int) (ua.getWidth()  * 0.94f));
            const int h = juce::jmin (1010, (int) (ua.getHeight() * 0.94f));
            centreWithSize (w, h);
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
