#include <stdint.h>

#include "third_party/nugget/psyqo/application.hh"
#include "third_party/nugget/psyqo/font.hh"
#include "third_party/nugget/psyqo/gpu.hh"
#include "third_party/nugget/psyqo/primitives/lines.hh"
#include "third_party/nugget/psyqo/primitives/rectangles.hh"
#include "third_party/nugget/psyqo/scene.hh"
#include "third_party/nugget/psyqo/simplepad.hh"

#include "cheat.hh"
#include "gui.hh"

namespace
{
    class FrontEnd final : public psyqo::Application
    {

        void prepare() override;
        void createScene() override;

    public:
        OpenCheat::GUI m_gui;
        psyqo::SimplePad m_input;

        int32_t m_selectedIndex = 0;
        int32_t m_listCount = 27;
    };

    class FrontEndScene final : public psyqo::Scene
    {
        void frame() override;
        void start(Scene::StartReason reason) override;

    private:
        enum DisplayState : uint8_t
        {
            MainMenu,
            CheatMenu
        } m_displayState;
    };

    FrontEnd frontEnd;
    FrontEndScene frontEndScene;

} // namespace

void FrontEnd::prepare()
{
    psyqo::GPU::Configuration config;
    config.set(psyqo::GPU::Resolution::W320)
        .set(psyqo::GPU::VideoMode::AUTO)
        .set(psyqo::GPU::ColorMode::C15BITS)
        .set(psyqo::GPU::Interlace::PROGRESSIVE);
    gpu().initialize(config);
}

void FrontEnd::createScene()
{
    m_gui.Initialize(gpu());
    m_input.initialize();
    pushScene(&frontEndScene);
}

void FrontEndScene::frame()
{
    psyqo::Color bg{{.r = 0, .g = 64, .b = 91}};
    gpu().clear(bg);

    frontEnd.m_gui.DrawBackground(frontEnd.gpu());
    switch (m_displayState)
    {
    case DisplayState::MainMenu:
        frontEnd.m_gui.DrawList(frontEnd.gpu(), frontEnd.m_selectedIndex, 27);
        break;

    case DisplayState::CheatMenu:
        frontEnd.m_gui.DrawTest(frontEnd.gpu(), frontEnd.m_selectedIndex, 1);
        break;
    }
}

void FrontEndScene::start(Scene::StartReason reason)
{
    frontEnd.m_input.setOnEvent(
        [this](const psyqo::SimplePad::Event &event)
        {
            if (event.pad == psyqo::SimplePad::Pad::Pad1)
            {
                if (event.type == psyqo::SimplePad::Event::ButtonPressed)
                {
                    switch (m_displayState)
                    {

                    case DisplayState::MainMenu:

                        switch (event.button)
                        {
                        case psyqo::SimplePad::Button::Cross:
                            m_displayState = DisplayState::CheatMenu;
                            break;
                        case psyqo::SimplePad::Button::Up:
                            frontEnd.m_selectedIndex == 0 ? frontEnd.m_selectedIndex = frontEnd.m_listCount - 1 : frontEnd.m_selectedIndex = frontEnd.m_selectedIndex - 1;
                            break;
                        case psyqo::SimplePad::Button::Down:
                            frontEnd.m_selectedIndex = (frontEnd.m_selectedIndex + 1) % frontEnd.m_listCount;
                            break;
                        default:
                            break;
                        }

                        break;

                    case DisplayState::CheatMenu:
                        switch (event.button)
                        {
                        case psyqo::SimplePad::Button::Cross:
                            OpenCheat::reboot();
                            break;

                        case psyqo::SimplePad::Button::Triangle:
                            m_displayState = DisplayState::MainMenu;
                            break;
                        }
                        break;
                    }
                }
            }
        });
}

int main() { return frontEnd.run(); }
