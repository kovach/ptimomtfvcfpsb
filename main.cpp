#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;

float strike(float t)
{
    if (t < 0.25)
        return t/0.25;
    if (t < 0.5)
        return 1-(t-0.25)/0.5;
    return 0.5 - (t-.5)/(1-.5) * .5;
}

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
    return v/100.f;
}
float getJoyY()
{
    float v = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
    v = sign(v) * fmax(0.f, fabs(v)-20.f) * 1.25f;
    return v/100.f;
}

bool getJumpButton()
{
    return sf::Joystick::isButtonPressed(0, 2);
}

enum Direction { None, Left, Right };

Direction getLungeButton()
{
    if (sf::Joystick::isButtonPressed(0, 4))
        return Left;
    if (sf::Joystick::isButtonPressed(0, 5))
        return Right;
    return None;
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
    return s.str();
}
string joystickPositions()
{
    std::ostringstream s;
    s << "joystick: "
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::X) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Y) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Z) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::R) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::U) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::V) << endl;
    return s.str();
}

enum JumpState {
    Ground, Crouching, Floating
};
string JumpStates[] = {
    "Ground", "Crouching", "Floating"
};
enum LungeState {
    Ready, Striking_Left, Striking_Right, Recovery
};
string LungeStates[] = {
    "Ready", "Striking_Left", "Striking_Right", "Recovery"
};
// TODO needed?
enum JumpEdge {
    StopJump, StartJump, NoEdge
};

const int fps = 60;
const float groundLevel = 750.;

class ControlState
{
public:
    JumpState jump;
    LungeState lunge;
    int facing;

    sf::Vector2f p;
    sf::Vector2f v;
    float radius;

    int crouchDuration;
    int lungeDuration;
    const static float maxCrouch = fps/10;
    const static float maxLunge = fps/12;
    const static float maxRecovery = fps/6;

    const static float maxVertical = 5000;
    const static float maxHorizontal = 3500;
    const static float gravity = 15000.f;
    const static float jumpForce = 19000.f;

    ControlState()
    {
        resetJump();
        jump = Floating;
        resetLunge();
        radius = 100.;
        p.x = 0.f;
        p.y = 0.f;
        v.x = 0.f;
        v.y = 0.f;

    }

    void resetJump()
    {
        jump = Ground;
        crouchDuration = 0;
    }
    void resetLunge()
    {
        lunge = Ready;
        lungeDuration = 0;
    }

    void stateUpdate()
    {
        // Update jump
        if (getJumpButton()) {
            if (jump == Ground) {
                jump = Crouching;
                crouchDuration = 0;
            }
            if (jump == Crouching && crouchDuration > maxCrouch) {
                jump = Floating;
            }
        } else {
            if (jump == Crouching)
                jump = Floating;
        }
        crouchDuration++;

        // Update attack
        Direction action = getLungeButton();
        switch (lunge) {
            case Ready:
                if (action == Left)
                    lunge = Striking_Left;
                if (action == Right)
                    lunge = Striking_Right;
                lungeDuration = 0;
                break;
            case Striking_Left:
            case Striking_Right:
                if (lungeDuration > maxLunge)
                    lunge = Recovery;
                break;
            case Recovery:
                if (lungeDuration > maxRecovery)
                    resetLunge();
                break;
        }
        lungeDuration++;
    }
    void update(float dt)
    {
        stateUpdate();
        getVelocity(dt);
        p.x += dt * v.x;
        p.y -= dt * v.y;

        if (p.y > groundLevel) {
            resetJump();
            p.y = groundLevel;
        }

    }

    void getVelocity(float dt)
    {
        switch(jump) {
            case Ground:
                if (lunge == Ready) {
                    v.x = getJoyX() * 2500;
                    facing = sign(v.x);
                }
                //vy -= gravity * dt;
                v.y = 0.;
                break;
            case Crouching:
                v.y += jumpForce * dt;
                //vy = maxVertical * strike((float)crouchDuration/maxCrouch);
                break;
            case Floating:
                float g = gravity;
                v.y -= g * dt;
                if (lunge == Striking_Right || lunge == Striking_Left)
                    v.y = 0.;
                break;
        }

        switch(lunge) {
            case Ready:
                break;
            case Striking_Left:
                v.x = -maxHorizontal * strike((float)lungeDuration/maxRecovery);
                break;
            case Striking_Right:
                v.x = maxHorizontal * strike((float)lungeDuration/maxRecovery);
                break;
            case Recovery:
                //vx = sign(vx) * maxHorizontal * strike(lungeDuration/maxRecovery);
                break;
        }
    }

    string toString(float time)
    {
        std::ostringstream s;
        s << JumpStates[jump] << endl << LungeStates[lunge];
        return s.str();
    }
};

bool collision(ControlState s1, ControlState s2)
{
    sf::Vector2f v = s1.p - s2.p;
    if (v.x*v.x + v.y*v.y < (s1.radius + s2.radius)^2) {
        return true;
    }
    return false;
}

class Thing : public sf::Drawable
{
public:
    Thing()
    {
        if (!s_font.loadFromFile("noto.ttf"))
            exit(22);
        m_text.setFont(s_font);
        m_text.setPosition(0.f, 0.f);
        m_text.setCharacterSize(36);
        m_text.setColor(sf::Color(255,128,0,255));

        if (!m_texture.loadFromFile("pt-crop.jpg"))
            exit(23);
        m_sprite.setTexture(m_texture);
        m_sprite.setScale(0.25f, 0.25f);
        m_sprite.setPosition(300, 300);

    }
    virtual ~Thing()
    {
    }
    void update(float time)
    {
        float dt = 1.0/60;

        controlState.update(dt);

        m_sprite.setPosition(controlState.p.x, controlState.p.y);

        std::ostringstream s;
        if (dt > 0.018f) {
            cout << dt << endl;
            exit(20);
        }
        s   << 1./dt << endl
            << joystickState() << endl
            << state() << endl
            << controlState.toString(time) << endl;

        m_text.setString(s.str());
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

    string state()
    {
        std::ostringstream s;
        s
            << "x: " << controlState.p.x << endl
            << "y: " << controlState.p.y << endl;
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

    sf::Vector2u dim = window.getSize();

    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(dim.x, 22));
    ground.setPosition(0.,groundLevel);
    ground.setFillColor(sf::Color(255,128,0,255));

    //for (float f = 0.; f < 1.; f += 0.01)
    //    cout << strike(f) << endl;
    //exit(0);

    int c = 0;

    window.setFramerateLimit(fps);

    Thing thing;

    sf::Clock clock;
    thing.update(clock.getElapsedTime().asSeconds());
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            //std::cout << c++ << "!\n";
            if (event.type == sf::Event::Closed)
                window.close();

        }
        thing.update(0);
        //clock.getElapsedTime().asSeconds();

        window.clear();
        window.draw(thing);
        window.draw(ground);
        window.display();
    }
    window.close();
    return 0;
}
