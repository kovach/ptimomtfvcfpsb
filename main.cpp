#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;

float sign(float x)
{
    if (x > 0.f)
        return 1;
    return -1;
}

float getJoyX()
{
    float v = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
    v = sign(v) * fmax(0.f, fabs(v)-20.f) * 1.25f;
    return v;
}
float getJoyY()
{
    float v = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
    v = sign(v) * fmax(0.f, fabs(v)-20.f) * 1.25f;
    return v;
}

bool getJumpButton()
{
    return sf::Joystick::isButtonPressed(0, 2);
}

string joystickState()
{
    bool connected = sf::Joystick::isConnected(0);
    unsigned int buttons = sf::Joystick::getButtonCount(0);
    bool hasX = sf::Joystick::hasAxis(0, sf::Joystick::X);
    bool pressed = sf::Joystick::isButtonPressed(0, 2);
    float position = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
    std::ostringstream s;
    
    for (int c = 0; c < buttons; c++) {
        s << "button " << c << " " << sf::Joystick::isButtonPressed(0, c) << endl;
    }
    //s << "joystick: "
    //    << "connected: " << connected << endl
    //    << "buttons: " << buttons << endl
    //    << "pressed: " << pressed << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::X) << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Y) << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Z) << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::R) << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::U) << endl
    //    << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::V) << endl;
    return s.str();
}

enum JumpState {
    Ground, Crouching, Jumping, Floating
};
enum JumpEdge {
    StopJump, StartJump, NoEdge
};

class ControlState
{
public:
    JumpState jump;
    float startCrouch;
    const static float maxCrouch = 0.1f;
    //const static float maxFloat = 2.2f;

    ControlState()
    {
        jump = Ground;
        startCrouch = 0.f;
    }

    void reset()
    {
        jump = Ground;
    }

    JumpEdge update(float time)
    {
        //if (jump == Floating && time - lastJump > maxFloat) {
        //    jump = Ground;
        //    return NoEdge;
        //}

        if (getJumpButton()) {
            if (jump == Ground) {
                jump = Crouching;
                startCrouch = time;
                //return StartJump;
            }
            if (jump == Crouching && time - startCrouch > maxCrouch) {
                jump = Floating;
            }
        } else {
            if (jump == Crouching)
                jump = Floating;
        }
        return NoEdge;
    }

    string toString(float time)
    {
        //update(time);
        switch (jump) {
            case Ground:
                return "Ground";
                break;
            case Crouching:
                return "Crouching";
                break;
            case Floating:
                return "Floating";
                break;
        }
    }
};

class Thing : public sf::Drawable
{
public:
    Thing()
    {
        vx = 0.f;
        vy = 0.f;
        x = 0.f;
        y = 0.f;
        if (!s_font.loadFromFile("noto.ttf"))
            exit(22);
        m_text.setFont(s_font);

        if (!m_texture.loadFromFile("pt-crop.jpg"))
            exit(23);
        m_sprite.setTexture(m_texture);
        m_sprite.setScale(0.25f, 0.25f);
        m_sprite.setPosition(300, 300);
    }
    virtual ~Thing()
    {
    }
    void update(float time, string str)
    {
        float dt = time - lastTime;
        lastTime = time;

        switch(controlState.jump) {
            case Ground:
                vx = getJoyX() * 25;
                break;
            case Crouching:
            case Floating:
                float goal = getJoyX() * 25;
                break;
        }

        controlState.update(time);
        switch(controlState.jump) {
            case Ground:
                vy -= gravity;
                break;
            case Crouching:
                vy += 90.f;
                break;
            case Floating:
                vy -= gravity;
                break;
        }

        x += dt * vx;
        y -= dt * vy;

        if (y > 500.f) {
            controlState.reset();
            y = 500.f;
            vy = 0.f;
        }


        m_sprite.setPosition(x, y);

        std::ostringstream s;
        s
            << joystickState() << endl
            << state() << endl
            << controlState.toString(time) << endl;

        m_text.setString(s.str());
        m_text.setPosition(0.f, 0.f);
        m_text.setCharacterSize(36);
        m_text.setColor(sf::Color(255,128,0,255));
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        target.draw(m_text, states);
        target.draw(m_sprite);
    }

private:
    ControlState controlState;

    sf::Font s_font;
    sf::Text m_text;

    sf::Texture m_texture;
    sf::Sprite m_sprite;

    float x; float y;
    float vx; float vy;
    float lastTime;
    const static float gravity = 60.f;

    string state()
    {
        std::ostringstream s;
        s
            << "x: " << x << endl
            << "y: " << y << endl;
        return s.str();
    }

    //static const sf::Font* s_font;
    //static void setFont(const sf::Font& font)
    //{
    //    s_font = &font;
    //}
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");

    //sf::CircleShape shape(100.f);
    //shape.setFillColor(sf::Color::Green);

    int c = 0;

    Thing thing;

    sf::Clock clock;
    thing.update(clock.getElapsedTime().asSeconds(), "hello world");
    while (window.isOpen())
    {
      sf::Event event;
      while (window.pollEvent(event))
      {
        //std::cout << c++ << "!\n";
        if (event.type == sf::Event::Closed)
          window.close();

      }
      thing.update(clock.getElapsedTime().asSeconds(), joystickState());

      window.clear();
      window.draw(thing);
      window.display();
    }


    return 0;
}
