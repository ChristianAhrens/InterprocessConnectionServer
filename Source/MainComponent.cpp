#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    m_ipEditor = std::make_unique<juce::TextEditor>();
    m_ipEditor->setText(m_ipAddress);
    m_ipEditor->onReturnKey = [=]() {
        m_ipAddress = juce::IPAddress(m_ipEditor->getText()).toString();
        restartConnection();
    };
    addAndMakeVisible(m_ipEditor.get());

    m_portEditor = std::make_unique<juce::TextEditor>();
    m_portEditor->setText(juce::String(m_port));
    m_portEditor->onReturnKey = [=]() {
        m_port = m_ipEditor->getText().getIntValue();
        restartConnection();
    };
    addAndMakeVisible(m_portEditor.get());

    m_connectedStatusButton = std::make_unique<juce::TextButton>();
    m_connectedStatusButton->setButtonText("C");
    m_connectedStatusButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::green);
    m_connectedStatusButton->setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::black);
    addAndMakeVisible(m_connectedStatusButton.get());

    m_dataLogEditor = std::make_unique<juce::TextEditor>();
    m_dataLogEditor->setMultiLine(true, false);
    addAndMakeVisible(m_dataLogEditor.get());

    m_connectionElmFlex = std::make_unique<juce::FlexBox>(juce::FlexBox::Direction::row,juce::FlexBox::Wrap::noWrap, juce::FlexBox::AlignContent::flexStart, juce::FlexBox::AlignItems::stretch, juce::FlexBox::JustifyContent::flexStart);
    m_connectionElmFlex->items = { 
        juce::FlexItem(*m_ipEditor.get()).withFlex(6),
        juce::FlexItem(*m_portEditor.get()).withFlex(4),
        juce::FlexItem(*m_connectedStatusButton.get()).withFlex(1)};
    
    m_mainFlex = std::make_unique<juce::FlexBox>(juce::FlexBox::Direction::column, juce::FlexBox::Wrap::noWrap, juce::FlexBox::AlignContent::flexStart, juce::FlexBox::AlignItems::stretch, juce::FlexBox::JustifyContent::flexStart);
    m_mainFlex->items = { 
        juce::FlexItem(*m_connectionElmFlex.get()).withMaxHeight(30).withFlex(1),
        juce::FlexItem(*m_dataLogEditor.get()).withFlex(9)};

    setSize (600, 400);

    m_connectionServer = std::make_unique<SimpleConnectionServer>();
    m_connectionServer->onConnectionMade = [=]() { m_connectedStatusButton->setToggleState(true, juce::NotificationType::dontSendNotification); };
    m_connectionServer->onConnectionLost = [=]() { m_connectedStatusButton->setToggleState(false, juce::NotificationType::dontSendNotification); };
    m_connectionServer->onMessageReceived = [=](const juce::MemoryBlock& message) { handleReceivedData(message); };

    restartConnection();
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    m_mainFlex->performLayout(getLocalBounds());
}

void MainComponent::restartConnection()
{
    m_connectionServer->stop();
    m_connectionServer->beginWaitingForSocket(m_port, m_ipAddress);
}

void MainComponent::handleReceivedData(const juce::MemoryBlock& message)
{
    if (m_dataLogEditor)
    {

        // AURA data testing
        ////////////////////

        std::function<std::uint32_t(const char*)> ReadUint32 = [=](const char* buffer)
            {
                return (((static_cast<std::uint8_t>(buffer[0]) << std::uint8_t(24)) & 0xff000000) +
                    ((static_cast<std::uint8_t>(buffer[1]) << std::uint8_t(16)) & 0x00ff0000) +
                    ((static_cast<std::uint8_t>(buffer[2]) << std::uint8_t(8)) & 0x0000ff00) +
                    static_cast<std::uint8_t>(buffer[3]));
            };

        if (message.isEmpty())
            return;

        if (m_dataLogList.size() >= 128)
            m_dataLogList.remove(127);

        auto packetId = ReadUint32(message.begin());
        if (packetId == 1 && message.getSize() == (4 * 4)) // listener pos, 3 float following
        {
            auto iX = ReadUint32(message.begin() + 4);
            auto iY = ReadUint32(message.begin() + 8);
            auto iZ = ReadUint32(message.begin() + 12);

            auto x = *reinterpret_cast<float*>(&iX);
            auto y = *reinterpret_cast<float*>(&iY);
            auto z = *reinterpret_cast<float*>(&iZ);

            auto dl = juce::String() << "listener pos: " << x << ";" << y << ";" << z;
            m_dataLogList.insert(0, dl);
        }
        else if (packetId == 2 && message.getSize() == (5 * 4)) // object pos, 1 int sourceId + 3 float following
        {
            auto iO = ReadUint32(message.begin() + 4);
            auto iX = ReadUint32(message.begin() + 8);
            auto iY = ReadUint32(message.begin() + 12);
            auto iZ = ReadUint32(message.begin() + 16);

            auto sourceId = int(iO);
            auto x = *reinterpret_cast<float*>(&iX);
            auto y = *reinterpret_cast<float*>(&iY);
            auto z = *reinterpret_cast<float*>(&iZ);

            auto dl = juce::String() << "obj. pos (" << sourceId << "): " << x << ";" << y << ";" << z;
            m_dataLogList.insert(0, dl);
        }
        else
        {
            juce::String stringyfiedMessage("unknown     : ");
            for (auto byte : message)
                stringyfiedMessage << juce::String::toHexString(byte) << " ";
            m_dataLogList.insert(0, stringyfiedMessage);
        }

        m_dataLogEditor->setText(m_dataLogList.joinIntoString("\n"));
    }
}
