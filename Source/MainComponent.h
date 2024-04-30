#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    void restartConnection();
    void handleReceivedData(const juce::MemoryBlock& message);

protected:
    class SimpleConnection : public juce::InterprocessConnection
    {
    public:
        ~SimpleConnection() override { disconnect(); };

        std::function<void()> onConnectionMade;
        std::function<void()> onConnectionLost;
        std::function<void(const juce::MemoryBlock&)> onMessageReceived;

    protected:
        void connectionMade() override { if (onConnectionMade) onConnectionMade(); };
        void connectionLost() override { if (onConnectionLost) onConnectionLost(); };
        void messageReceived(const juce::MemoryBlock& message) override { if (onMessageReceived) onMessageReceived(message); };
    };
    class SimpleConnectionServer : public juce::InterprocessConnectionServer
    {
    public:
        std::function<void()> onConnectionMade;
        std::function<void()> onConnectionLost;
        std::function<void(const juce::MemoryBlock&)> onMessageReceived;

    protected:
        juce::InterprocessConnection* createConnectionObject() override
        {
            m_connection = std::make_unique<SimpleConnection>();
            m_connection->onConnectionMade = [=] () { if (onConnectionMade) onConnectionMade(); };
            m_connection->onConnectionLost = [=]() { if (onConnectionLost) onConnectionLost(); };
            m_connection->onMessageReceived = [=](const juce::MemoryBlock& message) { if (onMessageReceived) onMessageReceived(message); };
            return m_connection.get();
        };

        std::unique_ptr<SimpleConnection>   m_connection;
    };

private:
    //==============================================================================
    std::unique_ptr<SimpleConnectionServer> m_connectionServer;
    int m_port{ 60123 };
    juce::String m_ipAddress{ "127.0.0.1" };
    
    //==============================================================================
    std::unique_ptr<juce::TextEditor>   m_ipEditor;
    std::unique_ptr<juce::TextEditor>   m_portEditor;
    std::unique_ptr<juce::TextButton>   m_connectedStatusButton;
    std::unique_ptr<juce::FlexBox>      m_connectionElmFlex;

    std::unique_ptr<juce::TextEditor>   m_dataLogEditor;
    juce::StringArray                   m_dataLogList;

    std::unique_ptr<juce::FlexBox>      m_mainFlex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
